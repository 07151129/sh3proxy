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

int repl_isFullscreen() {
    return (int)fullscreen;
}

int repl_416ba0(int unk0) {
    return 0;
}

int repl_41b250() {
    return 2;
}

int repl_setRefreshRate(int rate) {
    /* Disable refresh rate reset */
    return 0;
}

int repl_setSizeXY(int x, int y) {
    /* d3d8 device will be reset, so make sure
     * the mode is correct
     */
    if (x == resX && y == resY) {
        *(int*)0x72c780 = x; /* gSizeX */
        *(int*)0x72c784 = y; /* gSizeY */
        *(int*)0x72c788 = x; /* gWndResX */
        *(int*)0x72c78c = y; /* gWndResY */
        *(uint8_t*)0x72c655 = 1;
    }
    return 0;
}

bool patchVideoInit() {
    bool ret = true;

    /* Disable video reset on safe mode */
    uint8_t patchRes[] = {0xC7, 0x05, 0xE8, 0xC5, 0xBC, 0x00, (resX & 0xff), (resX & 0xff00) >> 0x8, (resX & 0xff0000) >> 0x10, (resX & 0xff000000) >> 0x18, /* mov resX, %ds:0xbcc5e8 */
        0xC7, 0x05, 0xEC, 0xC5, 0xBC, 0x00, (resY & 0xff), (resY & 0xff00) >> 0x8, (resY & 0xff0000) >> 0x10, (resY & 0xff000000) >> 0x18 /* mov resY, %ds:0xbcc5ec */};
    ret = patchText((void*)0x5f0993, patchRes, NULL, sizeof(patchRes));

    /* FIXME: dimensions can be different */
    /* FIXME: Does this actually do anything? */
    // uint8_t patchTex[] = {0xB8, (texRes & 0xff), (texRes & 0xff00) >> 0x8, (texRes & 0xff0000) >> 0x10, (texRes & 0xff000000) >> 0x18 /* mov texRes, %eax */};
    // ret = patchText((void*)0x5f0984, patchTex, NULL, sizeof(patchTex));

    uint8_t patchFs[] = {0xC6, 0x05, 0xFC, 0xC5, 0xBC, 0x0, fullscreen & 0xff, /* mov fullscreen, %ds:0xbcc5fc */};
    ret = patchText((void*)0x5f09bb, patchFs, NULL, sizeof(patchFs));

    uint8_t patchSettings[] = {0x25, 0xFF, 0xFF, 0xBF, 0xFF /* and $0xffbfffff, %eax */};
    ret = patchText((void*)0x5e6134, patchSettings, NULL, sizeof(patchSettings));

    return ret;
}

bool patchFOV(float projH) {
    bool ret = true;

    DWORD old_prot = 0;
    size_t sz = 0x43b649 - 0x43b63f+ 1;
    VirtualProtect((void*)0x43b63f, sz, PAGE_READWRITE, &old_prot);
    *(float*)0x43b645 = projH; /* initial value */
    VirtualProtect((void*)0x43b63f, sz, old_prot, NULL);

    VirtualProtect((void*)0x68f064, sizeof(float), PAGE_READWRITE, &old_prot);
    *(float*)0x68f064 = projH; /* zoom out threshold */
    VirtualProtect((void*)0x68f064, sizeof(float), old_prot, NULL);

    return ret;
}

bool patchEnableDOF() {
    bool ret = true;

    uint8_t patch[] = {0x90, 0x90, 0x90, 0x90, 0x90};
    ret = patchText((void*)0x41b9d4, patch, NULL, sizeof(patch));
    ret = patchText((void*)0x41ba7c, patch, NULL, sizeof(patch));

    *(uint8_t*)0x72c866 = 0; /* gEnableDOF */

    return ret;
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

    DWORD old_prot = 0;
    size_t sz = 0x48d8d1 - 0x48d88b + 1;
    VirtualProtect((void*)0x48d88b, sz, PAGE_READWRITE, &old_prot);

    uintptr_t toPatch[] = {0x48D891, 0x48D89B, 0x48D8A5, 0x48D8AF, 0x48D8B9, 0x48D8C3, 0x48D8CD, 0x48D8D7};
    for (size_t i = 0; i < sizeof(toPatch) / sizeof(*toPatch); i++)
        *(float*)toPatch[i] = res;

    VirtualProtect((void*)0x48d88b, sz, old_prot, NULL);

    return ret;
}

__attribute__((fastcall))
void printMat16(float* mat) {
    fprintf(stderr, "mat16 at 0x%x:\n"
                    "%f %f %f %f\n"
                    "%f %f %f %f\n"
                    "%f %f %f %f\n"
                    "%f %f %f %f\n", mat,
                    mat[0], mat[1], mat[2], mat[3],
                    mat[4], mat[5], mat[6], mat[7],
                    mat[8], mat[9], mat[10], mat[11],
                    mat[12], mat[13], mat[14], mat[15]);
}
