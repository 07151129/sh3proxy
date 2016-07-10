#ifndef VIDEO_H
#define VIDEO_H

int repl_getSizeX();
int repl_getSizeY();
bool patchVideoInit();
int repl_isFullscreen();
int repl_setSizeXY(int, int);
int repl_416ba0(int unk0);
int repl_setRefreshRate(int rate);

#endif
