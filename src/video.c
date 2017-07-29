#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

#include "patch.h"
#include "video.h"
#include "prefs.h"

int repl_getSizeX() {
    return resX;
}

int repl_getSizeY() {
    return resY;
}

int repl_getResX() {
    return resX;
}

int repl_getResY() {
    return resY;
}

int repl_isFullscreen() {
    return (int)fullscreen;
}

int repl_416ba0(int unk0) {
    return 0;
}

/* Fix jitter (FIXME) */
int repl_41b250_2() {
    return 2;
}

int repl_41b250_1() {
    return 1;
}

int repl_setRefreshRate(int rate) {
    /* Disable refresh rate reset */
    // *(uint32_t*)0x72c790 = rate;
    return 0;
}

int repl_setSizeXY(int x, int y) {
    /* Always override mode setting */
    // if (x == resX && y == resY) {
        *(int*)0x72c780 = resX; /* gSizeX */
        *(int*)0x72c784 = resY; /* gSizeY */
        *(int*)0x72c788 = resX; /* gWndResX */
        *(int*)0x72c78c = resY; /* gWndResY */
        *(uint8_t*)0x72c655 = 1;
        *(uint8_t*)0xbcc5fc = fullscreen; /* gFullscreen */
    // }
    return 0;
}

bool patchVideoInit(bool customRes) {
    bool ret = true;

    uint8_t patchFs[] = {0xC6, 0x05, 0xFC, 0xC5, 0xBC, 0x0, fullscreen & 0xff, /* mov fullscreen, %ds:0xbcc5fc */};
    ret = patchText((void*)0x5f09bb, patchFs, NULL, sizeof(patchFs));

    /* Ignore size change */
    uint8_t patchSettings[] = {0x25, 0xFF, 0xFF, 0xBF, 0xFF /* and $0xffbfffff, %eax */};
    ret = patchText((void*)0x5e6134, patchSettings, NULL, sizeof(patchSettings));

    if (customRes) {
        /* Ignore resolution change */
        patchSettings[3] = 0xff;
        patchSettings[4] = 0xfe; /* and $0xffefffff, %eax */
        ret = patchText((void*)0x5e3e75, patchSettings, NULL, sizeof(patchSettings));
    }

    return ret;
}

void repl_texInit();

bool patchTexInit() {
    bool ret = true;

    // uint8_t patchTexInit[] = {0x6a, 0x0, /* push 0 */};
    // ret = patchText((void*)0x461dbb, patchTexInit, NULL, sizeof(patchTexInit));
    
    /* FIXME: This patches tex init to use D3DPOOL_DEFAULT,
     * but usage of pool doesn't seem to be the bottleneck in first place*/
    replaceFuncAtAddr((void*)0x461dbd, repl_texInit, NULL);

    return ret;
}

bool patchPreviewRes(int32_t sz) {
    *(int32_t*)0x6b1434 = sz;
    *(int32_t*)0x6b1444 = sz;

    uint8_t patch[] = {(sz & 0xff), (sz & 0xff00) >> 0x8, (sz & 0xff0000) >> 0x10, (sz & 0xff000000) >> 0x18};

    bool ret = patchText((void*)0x41de57, patch, NULL, sizeof(patch));
    ret = patchText((void*)0x41e165, patch, NULL, sizeof(patch));

    float fsz = (float)sz;
    *(float*)0x6b0d00 = fsz;
    *(float*)0x6b0d30 = fsz;
    *(float*)0x6b0d34 = fsz;
    *(float*)0x6b0d4c = fsz;

    return ret;
}

bool patchFOV(float projH, float projV) {
    bool ret = true;

    DWORD old_prot = 0;
    size_t sz = 0x43b649 - 0x43b635+ 1;
    VirtualProtect((void*)0x43b635, sz, PAGE_READWRITE, &old_prot);
    *(float*)0x43b645 = projH; /* initial value */
    *(float*)0x43b63b = projV;
    VirtualProtect((void*)0x43b635, sz, old_prot, &old_prot);

    VirtualProtect((void*)0x68f064, sizeof(float), PAGE_READWRITE, &old_prot);
    *(float*)0x68f064 = projH; /* zoom out threshold */
    VirtualProtect((void*)0x68f064, sizeof(float), old_prot, &old_prot);

    return ret;
}

bool patchDOFResolution(int32_t res) {
    bool ret = true;

    int32_t texRes[] = {0x40, 0x200, res, res};
    memcpy((void*)0x6b1434, texRes, sizeof(texRes));
    memcpy((void*)0x6b1444, texRes, sizeof(texRes));

    return ret;
}

static uint8_t repl_41b9d0(uint8_t type) {
    return type;
}

bool patchDisableDOF() {
    return replaceFuncAtAddr((void*)0x41b9d0, repl_41b9d0, NULL);
}

bool patchCutscenesBorder() {
    bool ret = true;

    uint8_t patch[] = {0xd9, 0xee, /* fldz */
                       0x90, 0x90, 0x90, 0x90};

    ret = patchText((void*)0x41bb4b, patch, NULL, sizeof(patch));

    return ret;
}

bool patchShadows(float res) {
    bool ret = true;

    int ires = (int)res;
    uint8_t patch[] = {0x68, (ires & 0xff), (ires & 0xff00) >> 0x8, (ires & 0xff0000) >> 0x10, (ires & 0xff000000) >> 0x18 /* push res */};

    ret = patchText((void*)0x48b1b5, patch, NULL, sizeof(patch));
    ret = patchText((void*)0x48b1ba, patch, NULL, sizeof(patch));

    {
        DWORD old_prot = 0;
        size_t sz = 0x48d8d1 - 0x48d88b + 1;
        VirtualProtect((void*)0x48d88b, sz, PAGE_READWRITE, &old_prot);

        uintptr_t toPatch[] = {0x48D891, 0x48D89B, 0x48D8A5, 0x48D8AF, 0x48D8B9, 0x48D8C3, 0x48D8CD, 0x48D8D7};
        for (size_t i = 0; i < sizeof(toPatch) / sizeof(*toPatch); i++)
            *(float*)toPatch[i] = res;

        VirtualProtect((void*)0x48d88b, sz, old_prot, &old_prot);
    }

    {
        DWORD old_prot = 0;
        size_t sz = 0x48db98 - 0x48d661 + 1;
        VirtualProtect((void*)0x48d661, sz, PAGE_READWRITE, &old_prot);

        uintptr_t toPatch[] = {0x48D661, 0x48D696, 0x48D6D7, 0x48D6EE, 0x48DB4E, 0x48DB58, 0x48DB62, 0x48DB6C, 0x48DB76, 0x48DB80, 0x48DB8A, 0x48DB94};
        for (size_t i = 0; i < sizeof(toPatch) / sizeof(*toPatch); i++)
            *(float*)toPatch[i] = res;

        VirtualProtect((void*)0x48d661, sz, old_prot, &old_prot);
    }

    {
        DWORD old_prot = 0;
        size_t sz = 0x48b544 - 0x427235 + 1;
        VirtualProtect((void*)0x427235, sz, PAGE_READWRITE, &old_prot);

        uintptr_t toPatch[] = {0x48B29A, 0x48B29F, 0x48B53B, 0x48B540};
        for (size_t i = 0; i < sizeof(toPatch) / sizeof(*toPatch); i++)
            *(int*)toPatch[i] = ires;

        VirtualProtect((void*)0x427235, sz, old_prot, &old_prot);
    }

    return ret;
}

__attribute__((fastcall))
void printMat16(float* mat) {
    fprintf(stderr, "mat16 at 0x%x:\n"
                    "%f %f %f %f\n"
                    "%f %f %f %f\n"
                    "%f %f %f %f\n"
                    "%f %f %f %f\n", (uintptr_t)mat,
                    mat[0], mat[1], mat[2], mat[3],
                    mat[4], mat[5], mat[6], mat[7],
                    mat[8], mat[9], mat[10], mat[11],
                    mat[12], mat[13], mat[14], mat[15]);
}
