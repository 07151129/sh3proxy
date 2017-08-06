#include "anim.h"
#include "patch.h"

void repl_4c0890();

void anim_patch() {
    replaceFuncAtAddr((void*)0x4c089e, repl_4c0890, NULL);
}
