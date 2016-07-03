#ifndef PATCH_H
#define PATCH_H

#include <stdbool.h>
#include <stdint.h>

/* Replace first instruction of code at src with jump to repl */
bool replaceFuncAtAddr(void* src, void* repl, uint8_t bak[static 10]);

/* Replace data in .text. Back up is written to bak and should be at least as
 * large as replSz
 */
bool patchText(void* dst, const uint8_t* repl, uint8_t* bak, size_t replSz);

#endif
