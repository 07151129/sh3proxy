#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#define _USE_MATH_DEFINES
#include <tgmath.h>

#include "get_path.h"
#include "patch.h"
#include "prefs.h"
#include "sync.h"
#include "video.h"

bool disableSM, sh2Refs;
char savepathOverride[1024];
int resX, resY, texRes;
bool fullscreen;
HWND hWnd;

static inline
float toRad(float a) {
    return a * M_PI / 180.0f;
}

static inline
int clampPow2(int val, int min, int max) {
    int cnt = 0;

    for (int v = val; v > 0; v >>= 1)
        if (v & 1)
            cnt ++;

    if (cnt > 1) /* Must be pow2 */
        val &= (1 << (8 * sizeof(int) - __builtin_clz(val) - 1));

    /* Clamp */
    if (val > max)
        val = max;
    if (val < min)
        val = min;
    return val;
}

uint8_t* reloc_smWarn;
int repl_smWarn(int arg) {
    if (arg == 8)
        return 1;
    return ((int (*)(int))(void*)reloc_smWarn)(arg);
}

void debugLog(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void repl_setTransform();
void repl_calculateProjMatrix();
void repl_setWindowStyle();
void repl_setTexRes();
void repl_setPresentationIntervalWindowed();

/*
https://github.com/vrpn/vrpn/blob/master/server_src/
miles_sound_server/v6.0/Console.cpp#L170
 */
void redirectStdio() {
    // Create a console
    AllocConsole();
    AttachConsole(GetCurrentProcessId());

    int hConHandle;
    long lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;

    FILE *fp;
    const unsigned int MAX_CONSOLE_LINES = 500;
    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = MAX_CONSOLE_LINES;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen(hConHandle, "w");
    *stdout = *fp;
    setvbuf(stdout, NULL, _IONBF, 0);

    // redirect unbuffered STDIN to the console
    lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen(hConHandle, "r");
    *stdin = *fp;
    setvbuf(stdin, NULL, _IONBF, 0);

    // redirect unbuffered STDERR to the console
    lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen(hConHandle, "w");
    *stderr = *fp;
    setvbuf(stderr, NULL, _IONBF, 0);
}

static void init(HANDLE hModule) {
    if (GetPrivateProfileInt("Patches", "Console", 0, ".\\sh3proxy.ini") == 1)
        redirectStdio();

    fprintf(stderr, "sh3proxy: init\n");

    /* It looks like sh3.exe is not compatible with nx */
    if (!SetProcessDEPPolicy(0))
        fprintf(stderr, "sh3proxy: failed to disable dep\n");

    char filename[1024];
    GetModuleFileName(NULL, filename, sizeof(filename));
    if (strstr(filename, "sh3.exe") == NULL ||
        strncmp((char*)0x68A6DC, "SILENT HILL 3", strlen("SILENT HILL 3"))) {
        fprintf(stderr, "sh3proxy: target doesn't seem to be SH3, not patching anything\n");
        return;
    }

    fprintf(stderr, "sh3proxy: found SH3\n");

    disableSM = (GetPrivateProfileInt("Patches", "DisableSM", 0, ".\\sh3proxy.ini") == 1);
    sh2Refs = (GetPrivateProfileInt("Patches", "SH2Refs", 0, ".\\sh3proxy.ini") == 1);
    GetPrivateProfileString("Patches", "SavepathOverride", NULL, savepathOverride, sizeof(savepathOverride), ".\\sh3proxy.ini");
    bool whiteBorderFix = (GetPrivateProfileInt("Patches", "Win10WhiteBorderFix", 1, ".\\sh3proxy.ini") == 1);
    bool borderless = (GetPrivateProfileInt("Patches", "Borderless", 0, ".\\sh3proxy.ini") == 1);
    bool debugLogging = (GetPrivateProfileInt("Patches", "DebugLog", 0, ".\\sh3proxy.ini") == 1);
    bool UnlimitedAmmo = (GetPrivateProfileInt("Cheats", "UnlimitedAmmo", 0, ".\\sh3proxy.ini") == 1);
    bool GodMode = (GetPrivateProfileInt("Cheats", "GodMode", 0, ".\\sh3proxy.ini") == 1);

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

    if (disableSM) {
        size_t reloc_smWarn_sz = 90;
        reloc_smWarn = VirtualAlloc(NULL, reloc_smWarn_sz, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        // VirtualProtect(reloc_smWarn, reloc_smWarn_sz, PAGE_READWRITE, &prot);
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

        DWORD old_prot;
        VirtualProtect(reloc_smWarn, reloc_smWarn_sz, PAGE_EXECUTE_READ, &old_prot);
    }
    if (sh2Refs)
        replaceFuncAtAddr((void*)0x5e9760, repl_updateSH2InstallDir, NULL);
    if (whiteBorderFix) {
        uint8_t patchExStyle[] = {0x68, 0x0, 0x0, 0x0, 0x00, /* push WS_EX_LEFT */};
        patchText((void*)0x403042, patchExStyle, NULL, sizeof(patchExStyle));
    }
    if (borderless)
        replaceFuncAtAddr((void*)0x403032, repl_setWindowStyle, NULL);
    if (debugLogging)
        replaceFuncAtAddr((void*)0x41b6a0, debugLog, NULL);

    if (UnlimitedAmmo) { /* unlimited ammo - 66 FF 0C 4D 8E CA 12 07 */
        uint8_t patch[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

        patchText((void*)0x496a2e, patch, NULL, sizeof(patch));
    }
    
    if (GodMode) { /* God Mode - D9 9E 80 01 00 00, D9 96 80 01 00 00 */
        uint8_t patch[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

        patchText((void*)0x4ec96e, patch, NULL, sizeof(patch));
        patchText((void*)0x4eca95, patch, NULL, sizeof(patch));
    }

    { /* Don't re-create disp.ini */
        uint8_t patch[] = {0x90, 0x90, 0x90, 0x90, 0x90};
        patchText((void*)0x5f0a53, patch, NULL, sizeof(patch));
    }

    {
        bool enable = (GetPrivateProfileInt("FixJitter", "Enable", 1, ".\\sh3proxy.ini") == 1);
        bool showfps = (GetPrivateProfileInt("FixJitter", "ShowFPS", 1, ".\\sh3proxy.ini") == 1);

        if (enable) {
            sync_init(60, showfps);
            sync_patch();
        }
    }

    bool patchVideo = (GetPrivateProfileInt("Video", "Enable", 0, ".\\sh3proxy.ini") == 1);
    if (patchVideo) {
        resX = GetPrivateProfileInt("Video", "SizeX", 1280, ".\\sh3proxy.ini");
        resY = GetPrivateProfileInt("Video", "SizeY", 720, ".\\sh3proxy.ini");
        bool AutoDetectRes = (GetPrivateProfileInt("Video", "AutoDetectRes", 0, ".\\sh3proxy.ini") == 1);
        texRes = GetPrivateProfileInt("Video", "TexRes", 1024, ".\\sh3proxy.ini");
        fullscreen = (GetPrivateProfileInt("Video", "Fullscreen", 1, ".\\sh3proxy.ini") == 1);
        float fovX = (float)GetPrivateProfileInt("Video", "FovX", 90, ".\\sh3proxy.ini");
        float shadowRes = (float)GetPrivateProfileInt("Video", "ShadowRes", 1024, ".\\sh3proxy.ini");
        bool correctFog = (GetPrivateProfileInt("Video", "CorrectFog", 1, ".\\sh3proxy.ini") == 1);
        int DOFRes = GetPrivateProfileInt("Video", "DOFRes", 512, ".\\sh3proxy.ini");
        bool disableDOF = (GetPrivateProfileInt("Video", "DisableDOF", 0, ".\\sh3proxy.ini") == 1);
        bool disableCutscenesBorder = (GetPrivateProfileInt("Video", "DisableCutscenesBorder", 1, ".\\sh3proxy.ini") == 1);
        int previewRes = GetPrivateProfileInt("Video", "PreviewRes", 1024, ".\\sh3proxy.ini");
        bool matchRes = (GetPrivateProfileInt("Video", "MatchRes", 1, ".\\sh3proxy.ini") == 1);

        hWnd = GetActiveWindow();
        if (AutoDetectRes || !resX || !resY) {
            HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO info;

            info.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &info);

            resX = info.rcMonitor.right - info.rcMonitor.left;
            resY = info.rcMonitor.bottom - info.rcMonitor.top;
        }

        texRes = clampPow2(texRes, 256, 4096);
        shadowRes = (float)clampPow2(shadowRes, 128, 4096);

        float projH = sqrt(1.0f / (1.54f * tan(toRad(fovX) / 2.0f)));
        float projV = projH * sqrt((float) resX / resY * 0.8f);
        // fprintf(stderr, "projH: %f\n", projH);

        replaceFuncAtAddr((void*)0x4168e0, repl_getSizeX, NULL);
        replaceFuncAtAddr((void*)0x4168f0, repl_getSizeY, NULL);
        replaceFuncAtAddr((void*)0x416c90, repl_isFullscreen, NULL);
        replaceFuncAtAddr((void*)0x416ae0, repl_setSizeXY, NULL);
        replaceFuncAtAddr((void*)0x416ba0, repl_416ba0, NULL);
        replaceFuncAtAddr((void*)0x416b90, repl_setRefreshRate, NULL);
        patchFOV(projH, projV);
        // replaceFuncAtAddr((void*)0x67bba7, repl_setTransform, NULL);
        // replaceFuncAtAddr((void*)0x43beb0, repl_calculateProjMatrix, NULL);
        
        if (correctFog) {
            float ratio = (float)resX / (float)resY;

            DWORD old_prot = 0;
            VirtualProtect((void*)0x690634, 2 * sizeof(float), PAGE_READWRITE, &old_prot);
            *(float*)0x690634 = -ratio; /* gFogRateBack */
            *(float*)0x690638 = ratio; /* gFogRateFw */
            VirtualProtect((void*)0x690634, 2 * sizeof(float), old_prot, &old_prot);
        }

        if (!disableDOF)
            patchDOFResolution(clampPow2(DOFRes, 256, 4096));
        else
            patchDisableDOF();

        if (disableCutscenesBorder)
            patchCutscenesBorder();

        patchShadows((float)shadowRes);

        patchPreviewRes(clampPow2(previewRes, 64, 4096));

        if (matchRes) {
            replaceFuncAtAddr((void*)0x402b80, repl_getResX, NULL);
            replaceFuncAtAddr((void*)0x402b90, repl_getResY, NULL);
            replaceFuncAtAddr((void*)0x402e00, repl_setTexRes, NULL);
        }

        // patchTexInit();

        if (!patchVideoInit(matchRes))
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
        char path[MAX_PATH];

        memset(path, 0, sizeof(path));
        GetSystemDirectory(path, MAX_PATH);

        /* Append dll name */
        strncat(path, "\\"WRAP_DLL_NAME".dll", MAX_PATH);

        origDLL = LoadLibrary(path);
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
