#ifndef GET_PATH_H
#define GET_PATH_H

enum resource_t {
    DATA = 0,
    SAVE,
    MOVIE,
    SOUND
};

int repl_updateSH2InstallDir();
char* repl_getAbsPathImpl(enum resource_t type, const char*);

#endif
