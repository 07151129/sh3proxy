#include <stdio.h>
#include <windows.h>

#include "patch.h"
#include "get_path.h"

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved) {
        switch (reason) {
        case DLL_PROCESS_ATTACH: {
        /* Use CWD */
            uint8_t patch[] = {0x90, 0x90, 0x90, 0x90, 0x90};
            patchText((void*)0x5f120a, patch, NULL, sizeof(patch));
            replaceFuncAtAddr((void*)0x5f1130, repl_getAbsPathImpl, NULL);
            break;
        }
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

HRESULT WINAPI
DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
    static HINSTANCE origDLL;
    if (!origDLL) {
        char path[1024];
        GetSystemDirectoryA(path, sizeof(path));
        char* fullPath = calloc(strlen(path) + strlen("\\dinput8.dll") + 1, sizeof(char));
        snprintf(fullPath, strlen(path) + strlen("\\dinput8.dll") + 1, "%s\\dinput8.dll", path);

        origDLL = LoadLibrary(fullPath);
        if (!origDLL)
            ExitProcess(1);
    }

    static void* origAddr;
    if (!origAddr) {
        origAddr = GetProcAddress(origDLL, "DirectInput8Create");
        if (!origAddr) {
            fprintf(stderr, "dinput8_shim: failed to obtain original DirectInput8Create, exiting\n");
            ExitProcess(1);
        }
    }

    static HRESULT ret;
    if (!ret)
        ret = ((HRESULT (*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN))origAddr)(hinst, dwVersion, riidltf, ppvOut, punkOuter);

    return ret;
}
