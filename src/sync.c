#include <wtypes.h>
#include <mmsystem.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <tgmath.h>
#include <winuser.h>

#include "patch.h"
#include "sync.h"

/* Timer wrappers have been derived from SDL2.

Simple DirectMedia Layer
Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>
  
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
  
1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required. 
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

static uint32_t fps_desired;
static const uint32_t WAIT_THRESH_MS = 10;
#define FPS_REPORT_PERIOD 20
static const double FPS_REPORT_PERIOD_R = 1.0f / 30;
static uint32_t fps_mul, fps_total;

static bool usePerfCounter, showfps;
static LARGE_INTEGER perfCounter, counterFreq;
static uint32_t start_time, time_period;

void sync_init(uint32_t fps, bool show) {
    usePerfCounter = QueryPerformanceFrequency(&counterFreq);
    if (usePerfCounter)
        QueryPerformanceCounter(&perfCounter);
    else {
        timeBeginPeriod(1);
        start_time = timeGetTime();
    }
    fps_desired = fps;
    showfps = show;
}

uint32_t sync_getticks() {
    uint32_t now = 0;
    LARGE_INTEGER hires_now;

    if (usePerfCounter) {
        QueryPerformanceCounter(&hires_now);

        hires_now.QuadPart -= perfCounter.QuadPart;
        hires_now.QuadPart *= 1000;
        hires_now.QuadPart /= counterFreq.QuadPart;

        return (uint32_t)hires_now.QuadPart;
    } else
        now = timeGetTime();

    return (now - start_time);
}

void sync_delay(uint32_t ms) {
    Sleep(ms);
}

void sync_quit() {
    if (!usePerfCounter)
        timeEndPeriod(time_period);

    usePerfCounter = false;
    start_time = 0;
    fps_desired = 0, fps_mul = 0, fps_total = 0;
    showfps = false;
}

static inline
long double sync_period() {
    return 1000.0f / (long double)fps_desired;
}

static inline
uint32_t sync_nexttime() {
    long double ret = sync_period() * ++fps_mul;
    if (abs(round(ret) - ret) < 0.00001f)
        ret = round(ret);
    fps_total += ret;
    return ret;
}

static inline
void report_framerate(double avg_ms) {
    const char* game = "SILENT HILL 3";
    static char title[55];
    snprintf(title, sizeof(title) - 1, "[Average TPF: %7.3f ms / FPS: %7.3f] %s", avg_ms, 1000.0f / avg_ms, game);

    SetWindowText(*(HWND*)0x72bfe4, title);
}

void sync_dosync() {
    uint32_t niter_target = *(uint32_t*)0x72c7e8;
    if (!niter_target)
        return;

    uint32_t sleep_count = 0;
    if (niter_target > fps_desired)
        niter_target = fps_desired;

    static uint32_t nms_rem;
    static uint32_t prev_time;
    if (!prev_time)
        prev_time = sync_getticks();

    uint32_t tstart = sync_getticks();
    uint32_t niter = 0;

    for (; niter < niter_target || !sleep_count; niter++) {
        prev_time += (nms_rem + 1000) / fps_desired;
        nms_rem = (nms_rem + 1000) % fps_desired;

        if (prev_time > tstart)
            sleep_count++;

        // if (prev_time > tstart)
        //     fprintf(stderr, "%u ms to waste\n", prev_time - tstart);

        /**
         * Long time ahead; sleep for a fraction of the time
         * to account for the scheduler being imprecise
         */
        if (prev_time >= tstart + WAIT_THRESH_MS)
            sync_delay(1);

        // if (prev_time > sync_getticks())
        //     fprintf(stderr, "Spin for %u ms\n", prev_time - sync_getticks());

        /* Spin for the remaining time */
        while (prev_time > sync_getticks())
            ;
        tstart = sync_getticks();
    }

    *(uint32_t*)0x72c7f4 = niter;
    *(uint32_t*)0x72c7f8 = sleep_count - 1;

    if (showfps) {
        static uint32_t finished_time;
        static uint32_t nframes, sum_deltas;

        if (++nframes == FPS_REPORT_PERIOD) {
            report_framerate((double)sum_deltas * FPS_REPORT_PERIOD_R);
            sum_deltas = 0;
            nframes = 0;
        } else
            sum_deltas += tstart - finished_time;

        finished_time = tstart;
    }
}

static
int init_nop() {
    if (!fps_desired) /* Try to reuse refresh rate */
        fps_desired = *(uint32_t*)0x72c790;
    if (!fps_desired) /* The engine didn't get it */
        fps_desired = 60;

    return 0;
}

void sync_patch() {
    /* Disable timer init */
    replaceFuncAtAddr((void*)0x41b270, init_nop, NULL);

    /* Disable synchronization */
    uint8_t nops[] = {0x90, 0x90, 0x90, 0x90, 0x90};
    patchText((void*)0x41992d, nops, NULL, sizeof(nops));
    // patchText((void*)0x5f1853, nops, NULL, sizeof(nops));

    /* Force synchronization using sync_dosync before BeginScene */
    replaceFuncAtAddr((void*)0x41b5f0, sync_dosync, NULL);

    /* Release resources on exit */
    replaceFuncAtAddr((void*)0x613610, sync_quit, NULL);
}
