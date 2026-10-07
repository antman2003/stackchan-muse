/*
 * Speaks a reply on the chip with Espressif's Chinese TTS (Apache-2.0).
 * The voice itself is the xiaole set, mapped from the voice_data partition
 * rather than linked into the app. Other boards, and any reply with no
 * Chinese in it, leave speaking to the caller's silent caption pace.
 */

#include "muse_speak.h"

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "muse_speak";

#if CONFIG_MUSE_BOARD_M5STACK_CORES3

#include "esp_partition.h"
#include "esp_tts.h"
#include "esp_tts_voice_template.h"

#define SPEAK_SPEED 2
#define VOICE_BYTES 2938039
/* 64 KB pages. The file is 2,938,039 bytes; map that, not the spare tail. */
#define MAP_BYTES 0x2D0000

static esp_tts_handle_t s_tts;
static bool s_ready;
static bool s_failed;
static bool s_utt_done;
static int16_t *s_hold;
static int s_hold_cap;
static int s_hold_n;
static int s_hold_off;

static bool has_cjk(const char *s)
{
    const uint8_t *p = (const uint8_t *)s;
    while (*p) {
        uint32_t cp;
        if (*p < 0x80) {
            p++;
            continue;
        }
        if ((*p & 0xe0) == 0xc0 && p[1]) {
            cp = ((uint32_t)(*p & 0x1f) << 6) | (p[1] & 0x3f);
            p += 2;
        } else if ((*p & 0xf0) == 0xe0 && p[1] && p[2]) {
            cp = ((uint32_t)(*p & 0x0f) << 12) | ((uint32_t)(p[1] & 0x3f) << 6) | (p[2] & 0x3f);
            p += 3;
        } else if ((*p & 0xf8) == 0xf0 && p[1] && p[2] && p[3]) {
            cp = ((uint32_t)(*p & 0x07) << 18) | ((uint32_t)(p[1] & 0x3f) << 12) |
                 ((uint32_t)(p[2] & 0x3f) << 6) | (p[3] & 0x3f);
            p += 4;
        } else {
            p++;
            continue;
        }
        if (cp >= 0x4e00 && cp <= 0x9fff) {
            return true;
        }
    }
    return false;
}

/* Runs on an internal-RAM stack. esp_partition_mmap turns the cache off while
 * it rewrites the flash map. The chat task's stack is in PSRAM, which is
 * unreachable with the cache off, so doing this there resets the chip. */
static void map_voice(void)
{
    const esp_partition_t *part =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "voice_data");
    if (!part || part->size < VOICE_BYTES) {
        ESP_LOGW(TAG, "no voice_data partition; replies stay on screen");
        s_failed = true;
        return;
    }
    const void *map = NULL;
    esp_partition_mmap_handle_t mapped;
    if (esp_partition_mmap(part, 0, MAP_BYTES, ESP_PARTITION_MMAP_DATA, &map, &mapped) != ESP_OK ||
        !map || memcmp(map, "xiaole", 6) != 0) {
        ESP_LOGW(TAG, "voice data could not be mapped");
        s_failed = true;
        return;
    }
    esp_tts_voice_t *voice = esp_tts_voice_set_init(&esp_tts_voice_template, (void *)map);
    s_tts = voice ? esp_tts_create(voice) : NULL;
    if (!s_tts) {
        ESP_LOGW(TAG, "voice engine failed to start");
        s_failed = true;
        return;
    }
    s_ready = true;
    ESP_LOGI(TAG, "onboard voice ready, internal %u, psram %u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static void map_task(void *arg)
{
    SemaphoreHandle_t done = arg;
    map_voice();
    xSemaphoreGive(done);
    vTaskDeleteWithCaps(NULL);
}

static bool ensure(void)
{
    if (s_ready) {
        return true;
    }
    if (s_failed) {
        return false;
    }
    StaticSemaphore_t buf;
    SemaphoreHandle_t done = xSemaphoreCreateBinaryStatic(&buf);
    if (xTaskCreatePinnedToCoreWithCaps(map_task, "tts_map", 8192, done, 5, NULL, 0,
                                        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) != pdPASS) {
        ESP_LOGW(TAG, "no internal stack for the voice; replies stay on screen");
        s_failed = true;
        return false;
    }
    if (xSemaphoreTake(done, pdMS_TO_TICKS(8000)) != pdTRUE) {
        ESP_LOGW(TAG, "voice mapping timed out");
        s_failed = true;
        return false;
    }
    return s_ready;
}

bool muse_speak_begin(const char *utf8)
{
    s_utt_done = true;
    s_hold_n = s_hold_off = 0;
    if (!utf8 || !utf8[0] || !has_cjk(utf8) || !ensure()) {
        return false;
    }
    esp_tts_stream_reset(s_tts);
    if (!esp_tts_parse_chinese(s_tts, utf8)) {
        ESP_LOGW(TAG, "could not speak this reply; showing it instead");
        return false;
    }
    s_utt_done = false;
    return true;
}

int muse_speak_take(int16_t *dst, int max_samples)
{
    int out = 0;
    while (out < max_samples) {
        if (s_hold_off >= s_hold_n) {
            if (s_utt_done || !s_tts) {
                break;
            }
            int len = 0;
            short *pcm = esp_tts_stream_play(s_tts, &len, SPEAK_SPEED);
            if (len <= 0 || !pcm) {
                s_utt_done = true;
                break;
            }
            if (len > s_hold_cap) {
                int16_t *grown = heap_caps_realloc(s_hold, (size_t)len * sizeof(int16_t),
                                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (!grown) {
                    s_utt_done = true;
                    break;
                }
                s_hold = grown;
                s_hold_cap = len;
            }
            memcpy(s_hold, pcm, (size_t)len * sizeof(int16_t));
            s_hold_n = len;
            s_hold_off = 0;
        }
        int n = s_hold_n - s_hold_off;
        if (n > max_samples - out) {
            n = max_samples - out;
        }
        memcpy(dst + out, s_hold + s_hold_off, (size_t)n * sizeof(int16_t));
        s_hold_off += n;
        out += n;
    }
    return out;
}

#else

bool muse_speak_begin(const char *utf8)
{
    (void)utf8;
    return false;
}

int muse_speak_take(int16_t *dst, int max_samples)
{
    (void)dst;
    (void)max_samples;
    return 0;
}

#endif
