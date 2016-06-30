#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <direct.h>

#include "get_path.h"

char* repl_getAbsPathImpl(enum resource_t type, const char* relPath) {
    fprintf(stderr, "getAbsPathImpl: %d %s\n", type, type != SAVE ? relPath : NULL);

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
    else { /* SAVE type doesn't have relPath */
        len = strlen(cwd) + strlen("\\savedata\\") + 1;
        ret = calloc(len, sizeof(char));
        snprintf(ret, len, "%s\\savedata\\", cwd);
        static char* savePath;
        if (!savePath) {
            savePath = strdup(ret);
            *(char**)0xbcc614 = savePath;
            uint16_t unk[9]; /* File time as time_t, we discard this */
            if (savePath && ((int (*)(char*, void*))0x664aa8)(savePath, unk) == -1)
                _mkdir(savePath);
        }
    }

    fprintf(stderr, "ret: %s\n", ret);

    return ret;
}
