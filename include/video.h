#ifndef VIDEO_H
#define VIDEO_H

int repl_getSizeX();
int repl_getSizeY();
bool patchVideoInit();
bool patchFOV(float projH);
bool patchEnableDOF();
bool patchCutscenesBorder();
bool patchShadows(float res);
int repl_isFullscreen();
int repl_setSizeXY(int, int);
int repl_416ba0(int unk0);
int repl_41b2c0();
int repl_41b250_1();
int repl_41b250_2();
int repl_setRefreshRate(int rate);

#endif
