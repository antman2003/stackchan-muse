/*
 * M1 latency trace for one hold-to-talk turn.
 *
 * Shared because the voice task and the Hatch task both see the same turn.
 * This is a log, not a session or metrics service.
 *
 * T0 is the physical release (or the capture cap if release never comes).
 * T1 is the final voice-note body close after that release, not open_note at press.
 * T7 is not instrumented. M1_T7_PROXY is the first codec write of reply PCM
 * returning. That is driver acceptance, not speaker onset.
 */
#pragma once

#include <stdint.h>

enum {
    M1_T0 = 0,
    M1_T1,
    M1_T2,
    M1_T3,
    M1_T4,
    M1_T5,
    M1_T6,
    M1_T7_PROXY,
    M1_MARKS
};

/* Starts a trace. Closes an unfinished one as "replaced". */
void muse_latency_arm(uint32_t turn);
/* First call for each id wins. Marks other than T0 are ignored until T0. */
void muse_latency_mark(int id, const char *label);
/* A non-mark note, measured from T0. Ignored until T0. */
void muse_latency_note(const char *label);
/* One summary. Later calls do nothing until the next arm. */
void muse_latency_finish(const char *status);
