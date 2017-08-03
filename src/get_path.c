#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <direct.h>

#include <sys/stat.h>

#include "get_path.h"
#include "prefs.h"

int repl_updateSH2InstallDir() {
    *(uint8_t*)0xbcc250 = 1; /* gRegHasSH2 */
    *(uint8_t*)0xbcc251 = 1; /* gCheckedRegSH2 */
    
    return 1;
}

void* (*target_malloc)(size_t) = (void*)0x662f9b;

char* repl_getAbsPathImpl(enum resource_t type, const char* relPath) {
    char* ret = NULL;
    if (type > SOUND) { /* default case */
        ret = target_malloc(strlen(relPath) + strlen((char*)0x68a7b4));
        return ret;
    }

    static char* cwd;
    if (!cwd) {
        cwd = calloc(1024, sizeof(char));
        GetCurrentDirectoryA(1024, cwd);
    }
    
    size_t len = 0;
    if (type != SAVE) {
        len = strlen(cwd) + strlen(relPath) + strlen("\\") + 1;
        ret = target_malloc(len);
        snprintf(ret, len, "%s\\%s", cwd, relPath);
    }
    else {
        len = strlen("\\savedata\\") + strlen(relPath) + 1;
        bool override = (strlen(savepathOverride) > 0);
        
        if (override)
            len += strlen(savepathOverride);
        else
            len += strlen(cwd);

        ret = target_malloc(len);
        snprintf(ret, len, "%s\\savedata\\%s", override ? savepathOverride : cwd, relPath);
        static char* savePath;
        if (!savePath) { /* The first path is always a directory */
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
