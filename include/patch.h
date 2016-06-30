#ifndef PATCH_H
#define PATCH_H

#include <stdbool.h>
#include <stdint.h>

/* Replace first instruction of code at src with jump to repl */
bool replaceFuncAtAddr(void* src, void* repl, uint8_t bak[static 10]);

#endif
