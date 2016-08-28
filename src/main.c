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

float projH;

static inline
float toRad(float a) {
    return a * M_PI / 180.0f;
}

static inline
int clampPow2(int val, int min, int max) {
    int cnt = 0;
    __asm__ ("popcnt %1, %0;"
            : "=r"(cnt)
            : "r"(val)
            :);

    if (cnt > 1) /* Must be pow2 */
        val &= (1 << (8 * sizeof(int) - __builtin_clz(val) - 1));

    /* Clamp */
    if (val > max)
        val = max;
    if (val < min)
        val = min;
    return val;
}

__attribute__((section(".relocated")))
uint8_t reloc_smWarn[90];

int repl_smWarn(int arg) {
    if (arg == 8)
        return 1;
    return ((int (*)(int))(void*)reloc_smWarn)(arg);
}

void repl_setTransform();
void repl_calculateProjMatrix();
void repl_setWindowStyle();

static void init(HANDLE hModule) {
    fprintf(stderr, "sh3proxy: init\n");

    char filename[1024];
    GetModuleFileNameA(NULL, filename, sizeof(filename));
    if (strstr(filename, "sh3.exe") == NULL ||
        strncmp((char*)0x68A6DC, "SILENT HILL 3", strlen("SILENT HILL 3"))) {
        fprintf(stderr, "sh3proxy: target doesn't seem to be SH3, not patching anything\n");
        return;
    }

    fprintf(stderr, "sh3proxy: found SH3\n");

    useCwd = (GetPrivateProfileInt("Patches", "UseCWD", 0, ".\\sh3proxy.ini") == 1);
    disableSM = (GetPrivateProfileInt("Patches", "DisableSM", 0, ".\\sh3proxy.ini") == 1);
    sh2Refs = (GetPrivateProfileInt("Patches", "SH2Refs", 0, ".\\sh3proxy.ini") == 1);
    bool whiteBorderFix = (GetPrivateProfileInt("Patches", "Win10WhiteBorderFix", 1, ".\\sh3proxy.ini") == 1);
    bool borderless = (GetPrivateProfileInt("Patches", "Borderless", 0, ".\\sh3proxy.ini") == 1);

    if (useCwd) {
        /* FIXME for windows:
         * We allocate memory in repl_getAbsPathImpl,
         * which seems to be incompatible with whatever
         * allocator the game uses. This patch disables
         * the corresponding `free' call, at a cost of
         * small memory leak during initialisation.
         */
        uint8_t patch[] = {0x90, 0x90, 0x90, 0x90, 0x90};
        patchText((void*)0x5f120a, patch, NULL, sizeof(patch));
        replaceFuncAtAddr((void*)0x5f1130, repl_getAbsPathImpl, NULL);
    }
    if (disableSM) {
        VirtualProtect(reloc_smWarn, sizeof(reloc_smWarn), PAGE_READWRITE, NULL);
        replaceFuncAtAddr((void*)0x401000, repl_smWarn, reloc_smWarn);

        /* Relocate message routine: we still need
         * it for other types of messages
         */
        memcpy((uint8_t*)(reloc_smWarn + 6), (uint8_t*)0x401006, 90 - 6);
        uint32_t disp = (uint32_t)((uint8_t*)0x416930 - (uint8_t*)reloc_smWarn - 48 - 2);
        uint8_t patch[] = {0xe8, /* call *disp(%rip) */
                           (disp & 0xff),
                           (disp & 0xff00) >> 0x8,
                           (disp & 0xff0000) >> 0x10,
                           (disp & 0xff000000) >> 0x18
                           };
        memcpy((uint8_t*)(reloc_smWarn + 45), patch, sizeof(patch));
        VirtualProtect(reloc_smWarn, sizeof(reloc_smWarn), PAGE_EXECUTE_READ, NULL);
    }
    if (sh2Refs)
        replaceFuncAtAddr((void*)0x5e9760, repl_updateSH2InstallDir, NULL);
    if (whiteBorderFix) {
        uint8_t patchExStyle[] = {0x68, 0x0, 0x0, 0x0, 0x00, /* push WS_EX_LEFT */};
        patchText((void*)0x403042, patchExStyle, NULL, sizeof(patchExStyle));
    }
    if (borderless)
        replaceFuncAtAddr((void*)0x403032, repl_setWindowStyle, NULL);

    { /* Don't re-create disp.ini */
        uint8_t patch[] = {0x90, 0x90, 0x90, 0x90, 0x90};
        patchText((void*)0x5f0a53, patch, NULL, sizeof(patch));
    }

    {
        bool CapFPS = (GetPrivateProfileInt("FixJitter", "CapFPS", 0, ".\\sh3proxy.ini") == 1);
        bool AltFix = (GetPrivateProfileInt("FixJitter", "AltFix", 0, ".\\sh3proxy.ini") == 1);

        if (CapFPS)
            replaceFuncAtAddr((void*)0x41b250, repl_41b250_2, NULL);
        else if (AltFix)
            replaceFuncAtAddr((void*)0x41b250, repl_41b250_1, NULL);
    }

    bool patchVideo = (GetPrivateProfileInt("Video", "Enable", 0, ".\\sh3proxy.ini") == 1);
    if (patchVideo) {
        resX = GetPrivateProfileInt("Video", "SizeX", 1280, ".\\sh3proxy.ini");
        resY = GetPrivateProfileInt("Video", "SizeY", 720, ".\\sh3proxy.ini");
        texRes = GetPrivateProfileInt("Video", "TexRes", 1024, ".\\sh3proxy.ini");
        fullscreen = (GetPrivateProfileInt("Video", "Fullscreen", 1, ".\\sh3proxy.ini") == 1);
        float fovX = (float)GetPrivateProfileInt("Video", "FovX", 90, ".\\sh3proxy.ini");
        float shadowRes = (float)GetPrivateProfileInt("Video", "ShadowRes", 1024, ".\\sh3proxy.ini");
        bool correctFog = (GetPrivateProfileInt("Video", "CorrectFog", 1, ".\\sh3proxy.ini") == 1);
        int DOFRes = GetPrivateProfileInt("Video", "DOFRes", 512, ".\\sh3proxy.ini");
        bool disableDOF = (GetPrivateProfileInt("Video", "DisableDOF", 0, ".\\sh3proxy.ini") == 1);
        bool disableCutscenesBorder = (GetPrivateProfileInt("Video", "DisableCutscenesBorder", 1, ".\\sh3proxy.ini") == 1);

        texRes = clampPow2(texRes, 256, 4096);
        shadowRes = (float)clampPow2(shadowRes, 128, 4096);

        projH = sqrt(1.0f / (1.54f * tan(toRad(fovX) / 2.0f)));
        // fprintf(stderr, "projH: %f\n", projH);

        replaceFuncAtAddr((void*)0x4168e0, repl_getSizeX, NULL);
        replaceFuncAtAddr((void*)0x4168f0, repl_getSizeY, NULL);
        replaceFuncAtAddr((void*)0x416c90, repl_isFullscreen, NULL);
        replaceFuncAtAddr((void*)0x416ae0, repl_setSizeXY, NULL);
        replaceFuncAtAddr((void*)0x416ba0, repl_416ba0, NULL);
        replaceFuncAtAddr((void*)0x416b90, repl_setRefreshRate, NULL);
        patchFOV(projH);
        // replaceFuncAtAddr((void*)0x67bba7, repl_setTransform, NULL);
        // replaceFuncAtAddr((void*)0x43beb0, repl_calculateProjMatrix, NULL);
        
        if (correctFog) {
            float ratio = (float)resX / (float)resY;

            DWORD old_prot = 0;
            VirtualProtect((void*)0x690634, 2 * sizeof(float), PAGE_READWRITE, &old_prot);
            *(float*)0x690634 = -ratio; /* gFogRateBack */
            *(float*)0x690638 = ratio; /* gFogRateFw */
            VirtualProtect((void*)0x690634, 2 * sizeof(float), old_prot, NULL);
        }

        if (!disableDOF)
            patchDOFResolution(clampPow2(DOFRes, 256, 4096));
        else
            patchDisableDOF();

        if (disableCutscenesBorder)
            patchCutscenesBorder();

        patchShadows((float)shadowRes);

        // patchTexInit();

        if (!patchVideoInit())
            fprintf(stderr, "sh3proxy: video patching failed\n");
    }
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved) {
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

#ifdef WRAP_DINPUT
    #define WRAP_FN_RET_T HRESULT
    #define WRAP_FN_IFACE WRAP_FN_RET_T WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
    #define WRAP_FN_T WRAP_FN_RET_T(*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN)
    #define WRAP_FN_ARGS hinst, dwVersion, riidltf, ppvOut, punkOuter
    #define WRAP_FN_EXP "DirectInput8Create"
    #define WRAP_DLL_NAME "dinput8"
#else
    #define WRAP_FN_RET_T void*
    #define WRAP_FN_IFACE WRAP_FN_RET_T WINAPI d3d_create(long SDKVersion)
    #define WRAP_FN_T WRAP_FN_RET_T(*)(DWORD)
    #define WRAP_FN_ARGS SDKVersion
    #define WRAP_FN_EXP "Direct3DCreate8"
    #define WRAP_DLL_NAME "d3d8"
#endif

WRAP_FN_IFACE __asm__ ("_"WRAP_FN_EXP);

WRAP_FN_IFACE {
    static HINSTANCE origDLL;
    if (!origDLL) {
        char path[1024];
        GetSystemDirectoryA(path, sizeof(path));
        char name[] = WRAP_DLL_NAME".dll";

        char* fullPath = calloc(strlen(path) + strlen(name) + 1, sizeof(char));
        snprintf(fullPath, strlen(path) + strlen(name) + 1, "%s\\%s", path, name);

        origDLL = LoadLibrary(fullPath);
        if (!origDLL)
            ExitProcess(1);
    }

    static void* origAddr;
    if (!origAddr) {
        char func[] = WRAP_FN_EXP;

        origAddr = GetProcAddress(origDLL, func);
        if (!origAddr) {
            fprintf(stderr, "sh3proxy: failed to obtain original %s, exiting\n", func);
            ExitProcess(1);
        }
    }

    static WRAP_FN_RET_T ret;
    if (!ret)
        ret = ((WRAP_FN_T)origAddr)(WRAP_FN_ARGS);

    return ret;
}
