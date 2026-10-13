#include "muse_latency.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static const char *TAG = "m1";

static const char *const s_tag[M1_MARKS] = {
    "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7_PROXY",
};

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t s_turn;
static int64_t s_t0;
static int64_t s_us[M1_MARKS];
static bool s_seen[M1_MARKS];
static bool s_open;
static bool s_summarized;

static void log_ms(uint32_t turn, const char *tag, const char *label, int64_t delta_us)
{
    int64_t tenths = delta_us / 100;
    if (delta_us < 0 && delta_us % 100) {
        tenths -= 1;
    }
    int64_t whole = tenths / 10;
    int frac = (int)(tenths % 10);
    if (frac < 0) {
        frac = -frac;
    }
    ESP_LOGI(TAG, "[M1 LATENCY] turn=%u %s %s %+lld.%d ms",
             (unsigned)turn, tag, label ? label : "", (long long)whole, frac);
}

static void summary_locked_copy(uint32_t *turn, int64_t us[M1_MARKS], bool seen[M1_MARKS])
{
    *turn = s_turn;
    memcpy(us, s_us, sizeof(s_us));
    memcpy(seen, s_seen, sizeof(s_seen));
}

static bool t0_ok(const bool seen[M1_MARKS])
{
    return seen[M1_T0];
}

void muse_latency_finish(const char *status)
{
    uint32_t turn = 0;
    int64_t us[M1_MARKS];
    bool seen[M1_MARKS];
    bool emit = false;

    portENTER_CRITICAL(&s_mux);
    if (s_open && !s_summarized) {
        s_summarized = true;
        s_open = false;
        summary_locked_copy(&turn, us, seen);
        emit = true;
    }
    portEXIT_CRITICAL(&s_mux);
    if (!emit) {
        return;
    }

    ESP_LOGI(TAG, "[M1 LATENCY] turn=%u SUMMARY status=%s", (unsigned)turn, status ? status : "");
    char line[192];
    int n = 0;
    for (int i = 0; i < M1_MARKS && n < (int)sizeof(line); i++) {
        if (!seen[i] || !t0_ok(seen)) {
            continue;
        }
        int64_t delta = us[i] - us[M1_T0];
        int64_t tenths = delta / 100;
        int frac = (int)(tenths % 10);
        if (frac < 0) {
            frac = -frac;
        }
        n += snprintf(line + n, sizeof(line) - n, " %s=%lld.%d", s_tag[i],
                      (long long)(tenths / 10), frac);
    }
    ESP_LOGI(TAG, "[M1 LATENCY] turn=%u marks%s", (unsigned)turn, n ? line : " none");

    n = 0;
    int prev = -1;
    for (int i = 0; i < M1_MARKS && n < (int)sizeof(line); i++) {
        if (!seen[i]) {
            continue;
        }
        if (prev >= 0) {
            int64_t delta = us[i] - us[prev];
            int64_t tenths = delta / 100;
            int frac = (int)(tenths % 10);
            if (frac < 0) {
                frac = -frac;
            }
            n += snprintf(line + n, sizeof(line) - n, " %s-%s=%lld.%d", s_tag[prev], s_tag[i],
                          (long long)(tenths / 10), frac);
        }
        prev = i;
    }
    if (n) {
        ESP_LOGI(TAG, "[M1 LATENCY] turn=%u gaps%s", (unsigned)turn, line);
    }
}

void muse_latency_arm(uint32_t turn)
{
    bool close = false;
    portENTER_CRITICAL(&s_mux);
    close = s_open && !s_summarized;
    portEXIT_CRITICAL(&s_mux);
    if (close) {
        muse_latency_finish("replaced");
    }
    portENTER_CRITICAL(&s_mux);
    memset(s_seen, 0, sizeof(s_seen));
    memset(s_us, 0, sizeof(s_us));
    s_t0 = 0;
    s_turn = turn;
    s_open = true;
    s_summarized = false;
    portEXIT_CRITICAL(&s_mux);
}

void muse_latency_mark(int id, const char *label)
{
    bool emit = false;
    bool done = false;
    uint32_t turn = 0;
    int64_t delta = 0;

    if (id < 0 || id >= M1_MARKS) {
        return;
    }
    portENTER_CRITICAL(&s_mux);
    if (s_open && !s_summarized && !s_seen[id]) {
        int64_t t = esp_timer_get_time();
        if (id == M1_T0) {
            s_t0 = t;
            s_seen[id] = true;
            s_us[id] = t;
            emit = true;
            delta = 0;
            turn = s_turn;
        } else if (s_seen[M1_T0]) {
            s_seen[id] = true;
            s_us[id] = t;
            emit = true;
            delta = t - s_t0;
            turn = s_turn;
            done = id == M1_T7_PROXY;
        }
    }
    portEXIT_CRITICAL(&s_mux);
    if (!emit) {
        return;
    }
    log_ms(turn, s_tag[id], label, delta);
    if (done) {
        muse_latency_finish("complete");
    }
}

void muse_latency_note(const char *label)
{
    uint32_t turn = 0;
    int64_t delta = 0;
    bool emit = false;
    portENTER_CRITICAL(&s_mux);
    if (s_open && !s_summarized && s_seen[M1_T0]) {
        emit = true;
        turn = s_turn;
        delta = esp_timer_get_time() - s_t0;
    }
    portEXIT_CRITICAL(&s_mux);
    if (emit) {
        log_ms(turn, "note", label, delta);
    }
}
