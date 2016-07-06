#include <windows.h>

#include "patch.h"

static uintptr_t jmp_ptrs[128];

bool replaceFuncAtAddr(void* src, void* repl, uint8_t bak[static 6]) {
    if (!src || !repl)
        return false;

    static size_t i;
    /* No room in ptr table to store repl */
    if (i >= sizeof(jmp_ptrs) / sizeof(*jmp_ptrs))
        return false;

    jmp_ptrs[i] = (uint32_t)repl;
    uint32_t ptr = (uint32_t)&jmp_ptrs[i];
    i++;

    uint8_t patch[] = {0xff, 0x25, /* jmp *ptr*/
                       ptr & 0xff,
                       (ptr & 0xff00) >> 0x8,
                       (ptr & 0xff0000) >> 0x10,
                       (ptr & 0xff000000) >> 0x18,
                       };

    patchText(src, patch, bak, sizeof(patch));

    return true;
}

bool patchText(void* dst, const uint8_t* repl, uint8_t* bak, size_t replSz) {
    if (!dst || !repl)
        return false;

    DWORD old_prot = 0;

    if (!VirtualProtect(dst, replSz, PAGE_READWRITE, &old_prot))
        return false;

    if (bak)
        memcpy(bak, dst, replSz);
    memcpy(dst, repl, replSz);

    VirtualProtect(dst, replSz, old_prot, NULL);

    return true;
}
