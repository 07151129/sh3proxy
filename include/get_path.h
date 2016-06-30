#ifndef GET_PATH_H
#define GET_PATH_H

enum resource_t {
    DATA = 0,
    SAVE,
    MOVIE,
    SOUND
};

char* repl_getAbsPathImpl(enum resource_t type, const char*);

#endif
