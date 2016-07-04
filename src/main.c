#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#define _USE_MATH_DEFINES
#include <tgmath.h>

#include "patch.h"
#include "get_path.h"
#include "prefs.h"
#include "video.h"

bool useCwd, disableSM, sh2Refs;
int resX, resY, texRes;
bool fullscreen;

float projH, projV;

static inline
float toRad(float a) {
    return a * M_PI / 180.0f;
}

static int repl_smWarn(int arg) {
    return 1;
}

void repl_setTransform();

static void init(HANDLE hModule) {
    fprintf(stderr, "sh3proxy: init\n");

    /* FIXME: More reliable way of checking */
    if (strncmp((char*)0x68A6DC, "SILENT HILL 3", strlen("SILENT HILL 3"))) {
        fprintf(stderr, "sh3proxy: target doesn't seem to be SH3, not patching anything\n");
        return;
    }

    fprintf(stderr, "sh3proxy: found SH3\n");

    useCwd = (GetPrivateProfileInt("Patches", "UseCWD", 0, ".\\sh3proxy.ini") == 1);
    disableSM = (GetPrivateProfileInt("Patches", "DisableSM", 0, ".\\sh3proxy.ini") == 1);
    sh2Refs = (GetPrivateProfileInt("Patches", "SH2Refs", 0, ".\\sh3proxy.ini") == 1);

    if (useCwd)
        replaceFuncAtAddr((void*)0x5f1130, repl_getAbsPathImpl, NULL);
    if (disableSM)
        replaceFuncAtAddr((void*)0x401000, repl_smWarn, NULL);
    if (sh2Refs) /* TODO: Test this */
        replaceFuncAtAddr((void*)0x5e9760, repl_updateSH2InstallDir, NULL);

    bool patchVideo = (GetPrivateProfileInt("Video", "Enable", 0, ".\\sh3proxy.ini") == 1);
    if (patchVideo) {
        resX = GetPrivateProfileInt("Video", "SizeX", 1280, ".\\sh3proxy.ini");
        resY = GetPrivateProfileInt("Video", "SizeY", 720, ".\\sh3proxy.ini");
        texRes = GetPrivateProfileInt("Video", "TexRes", 1024, ".\\sh3proxy.ini");
        fullscreen = (GetPrivateProfileInt("Video", "Fullscreen", 1, ".\\sh3proxy.ini") == 1);
        float fovX = (float)GetPrivateProfileInt("Video", "FovX", 90, ".\\sh3proxy.ini");

        int cnt = 0;
        __asm__ ("popcnt %1, %0;"
                : "=r"(cnt)
                : "r"(texRes)
                :);

        if (cnt > 1) /* Must be pow2 */
            texRes &= (1 << (8 * sizeof(int) - __builtin_clz(texRes) - 1));

        /* Clamp */
        if (texRes > 4096)
            texRes = 4096;
        if (texRes < 256)
            texRes = 256;
        
        projH = 1.0f / tan(toRad(fovX / 2.0f));
        projV = -1.0f / ((float)resY / (float)resX * tan(toRad(fovX / 2.0f)));

        replaceFuncAtAddr((void*)0x4168e0, repl_getSizeX, NULL);
        replaceFuncAtAddr((void*)0x4168f0, repl_getSizeY, NULL);
        replaceFuncAtAddr((void*)0x416c90, repl_isFullscreen, NULL);
        replaceFuncAtAddr((void*)0x416ae0, repl_setSizeXY, NULL);
        replaceFuncAtAddr((void*)0x67bba7, repl_setTransform, NULL);

        if (!patchVideoInit())
            fprintf(stderr, "sh3proxy: video patching failed\n");
    }
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved) {
    (void)lpReserved;

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            init(hModule);
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        default:
            break;
    }

    return TRUE;
}

void* WINAPI
d3d_create(long SDKVersion) {
    // fprintf(stderr, "Direct3DCreate8\n");
    static HINSTANCE origDLL;
    if (!origDLL) {
        char path[1024];
        GetSystemDirectoryA(path, sizeof(path));
        char* fullPath = calloc(strlen(path) + strlen("\\d3d8.dll") + 1, sizeof(char));
        snprintf(fullPath, strlen(path) + strlen("\\d3d8.dll") + 1, "%s\\d3d8.dll", path);

        origDLL = LoadLibrary(fullPath);
        if (!origDLL)
            ExitProcess(1);
    }

    static void* origAddr;
    if (!origAddr) {
        origAddr = GetProcAddress(origDLL, "Direct3DCreate8");
        if (!origAddr) {
            fprintf(stderr, "sh3proxy: failed to obtain original Direct3DCreate8, exiting\n");
            ExitProcess(1);
        }
    }

    static void* ret;
    if (!ret)
        ret = ((void* (*)(UINT))origAddr)(SDKVersion);

    return ret;
}
