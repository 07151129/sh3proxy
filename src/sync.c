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
static const uint32_t FPS_COMPENSATE_THRESH = 50;
static const uint32_t FPS_REPORT_PERIOD_MS = 1500;
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

void report_framerate() {
    static uint32_t start_time, prev_time;
    static uint32_t nframes, sum_deltas;

    uint32_t now = sync_getticks();

    nframes++;

    if (now > start_time + FPS_REPORT_PERIOD_MS) {
        double avg_ms = (double)sum_deltas / nframes;

        static char* game = (void*)0x68a6dc;
        static char title[55];
        snprintf(title, sizeof(title) - 1, "[Average TPF: %7.3f ms / FPS: %7.3f] %s", avg_ms, 1000.0f / avg_ms, game);
        SetWindowText(*(HWND*)0x72bfe4, title);

        start_time = now;
        nframes = 0;
        sum_deltas = 0;   
    } else {
        sum_deltas += now - prev_time;
        prev_time = now;
    }
}

__attribute__ ((cdecl))
void sync_dosync() {
    int32_t sleep_count = 0;

    static uint32_t nms_rem;
    static uint32_t prev_time;
    if (!prev_time)
        prev_time = sync_getticks();

    uint32_t tstart = sync_getticks();

    while (!sleep_count) {
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
}

int init_nop() {
    if (!fps_desired) /* Try to reuse refresh rate */
        fps_desired = *(uint32_t*)0x72c790;
    if (!fps_desired) /* The engine didn't get it */
        fps_desired = 60;

    return 0;
}

void compute_slowdown_coeff() {

}

void report_framerate_trampl(), scale_event_rate_trampl();

__attribute__ ((cdecl))
void scale_event_rate() {
    static uint32_t prev;

    uint32_t now = sync_getticks();
    uint32_t tdiff = now - prev;

    float rate = 1.0f;
    if (tdiff > 1000 / FPS_COMPENSATE_THRESH)
        rate = (float)tdiff/ 17.0f;

    prev = now;

    *(uint32_t*)0x70e67c0 = (uint32_t)rate;
    *(uint32_t*)0x70e67a4 = (uint32_t)rate;
    *(float*)0x70e67a8 = rate;
    *(float*)0x70e67b4 = rate;
    *(float*)0x70e67ac = rate * 0.016666666666666666f; /* gRendertimeScaled */
    *(float*)0x70e67c4 = 60.0f / rate; /* gFPSScaled */
}

void sync_patch() {
    /* Disable timer init */
    replaceFuncAtAddr((void*)0x41b270, init_nop, NULL);

    /* Disable synchronization */
    uint8_t nops[] = {0x90, 0x90, 0x90, 0x90, 0x90};
    patchText((void*)0x41992d, nops, NULL, sizeof(nops));

    /* Disable event rate scaling from event loop */
    replaceFuncAtAddr((void*)0x5f1a73, scale_event_rate_trampl, NULL);

    replaceFuncAtAddr((void*)0x5f15e0, compute_slowdown_coeff, NULL);
    
    if (showfps)
        replaceFuncAtAddr((void*)0x41995a, report_framerate_trampl, NULL);

    /* Force synchronization using sync_dosync before BeginScene */
    replaceFuncAtAddr((void*)0x41b5f0, sync_dosync, NULL);

    /* Release resources on exit */
    replaceFuncAtAddr((void*)0x613610, sync_quit, NULL);
}
