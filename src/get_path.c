#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <direct.h>

#include "get_path.h"

char* repl_getAbsPathImpl(enum resource_t type, const char* relPath) {
    // fprintf(stdout, "getAbsPathImpl: %d %s\n", type, relPath);

    if (type > SOUND) { /* default case, will abort */
        return (char*)0x68a7b4;
    }

    static char* cwd;
    if (!cwd) {
        cwd = calloc(1024, sizeof(char));
        GetCurrentDirectoryA(1024, cwd);
    }

    /* FIXME: Locking */
    
    size_t len = 0;
    char* ret = NULL;
    if (type != SAVE) {
        len = strlen(cwd) + strlen(relPath) + strlen("\\") + 1;
        ret = calloc(len, sizeof(char));
        snprintf(ret, len, "%s\\%s", cwd, relPath);
    }
    else {
        len = strlen(cwd) + strlen("\\savedata\\") + strlen(relPath) + 1;
        ret = calloc(len, sizeof(char));
        snprintf(ret, len, "%s\\savedata\\%s", cwd, relPath);
        static char* savePath;
        if (!savePath) {
            savePath = strdup(ret);
            *(char**)0xbcc614 = savePath;
            uint16_t unk[9]; /* File time as time_t, we discard this */
            if (savePath && ((int (*)(char*, void*))0x664aa8)(savePath, unk) == -1)
                _mkdir(savePath);
        }
    }

    // fprintf(stderr, "ret: %s\n", ret);

    return ret;
}
