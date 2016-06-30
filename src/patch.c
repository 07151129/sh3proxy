#include <windows.h>

#include "patch.h"

static uintptr_t jmp_ptrs[128];

static long getpagesize(void) {
    static long sz = 0;
    if (!sz) {
        SYSTEM_INFO inf;
        GetSystemInfo (&inf);
        sz = inf.dwPageSize;
    }
    return sz;
}

bool replaceFuncAtAddr(void* src, void* repl, uint8_t bak[static 6]) {
    if (!src || !repl)
        return false;

    static size_t i;
    /* No room in ptr table to store repl */
    if (i >= sizeof(jmp_ptrs) / sizeof(*jmp_ptrs))
        return false;

    /* We only need the first page */
    void* src_aligned = (void*)((uintptr_t)src & ~(getpagesize() - 1));
    DWORD old_prot = 0;

    if (!VirtualProtect(src_aligned, getpagesize(), PAGE_READWRITE, &old_prot))
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

    if (bak)
        memcpy(bak, src, sizeof(bak));
    memcpy(src, patch, sizeof(patch));

    VirtualProtect(src_aligned, getpagesize(), PAGE_EXECUTE_READ, &old_prot);

    return true;
}
