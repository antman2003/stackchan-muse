#pragma once

#include <stdbool.h>
#include <stdint.h>

/* On-chip Chinese speech. begin() is false when this build has no voice,
 * the text has no Chinese, or the voice data is missing: the caller then
 * shows the reply at reading pace, as before. */
bool muse_speak_begin(const char *utf8);

/* Next samples of the utterance started by begin(). 0 means it is finished. */
int muse_speak_take(int16_t *dst, int max_samples);
