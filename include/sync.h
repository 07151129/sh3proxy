#ifndef SYNCH_H
#define SYNC_H

#include <wtypes.h>
#include <stdint.h>

void sync_init(uint32_t fps, bool showfps);
void sync_quit();
uint32_t sync_getticks();
void sync_delay(uint32_t ms);
void sync_patch();

#endif
