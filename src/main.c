#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#include "patch.h"
#include "get_path.h"
#include "prefs.h"

bool useCwd, disableSM, sh2Refs;

static int repl_sm(int arg) {
    return 1;
}

static int repl_updateSH2InstallDir() {
    *(uint8_t*)0xbcc250 = 1; /* gRegHasSH2 */
    *(uint8_t*)0xbcc251 = 1; /* gCheckedRegSH2 */
    
    return 1;
}

static void init(HANDLE hModule) {
    fprintf(stderr, "sh3proxy: init\n");

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
        replaceFuncAtAddr((void*)0x401000, repl_sm, NULL);
    if (sh2Refs)
        replaceFuncAtAddr((void*)0x5e9760, repl_updateSH2InstallDir, NULL);
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
