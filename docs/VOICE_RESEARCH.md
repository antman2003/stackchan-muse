# Voice research

> Moved from the workspace lab notebook on 2026-10-09.
> Device identifiers (SSID, LAN address, MAC, BLE name, and a personal name in a sample reply) are replaced with placeholders.
> Cross-links that pointed at old README anchors are rewritten below where they would break.

## Status

**prototype-only.** The on-device esp-tts / xiaole path speaks Chinese and is wired into the Muse chat turn. It is not a completed product feature.

Known limits, as of 2026-10-09:

- The voice is mechanical and is not acceptable for the final product.
- English speech is not properly supported on this path. English replies stay on screen.
- End-to-end latency can exceed 10 seconds.
- The final architecture may replace this implementation. Do not assume xiaole remains the speech engine.

Latency milestones should time these points, not "faster":

| Mark | Meaning |
|---|---|
| T0 | End of user speech, or touch release |
| T1 | Muse request sent |
| T2 | First response data |
| T3 | Full reply text available |
| T4 | TTS requested |
| T5 | First PCM or audio available |
| T6 | First PCM or audio submitted to playback |
| T7 | First audible audio |

## On-device xiaole prototype

### 5. Spoken replies [4] — on the robot as of 2026-10-06

**Status: prototype-only.** "On the robot" means the xiaole path runs. It is not a finished product feature. See the status section above.

Chinese replies are spoken on the chip with Espressif's **xiaole** voice, from
the Apache-2.0 [esp-tts](https://github.com/espressif/esp-sr/tree/master/esp-tts)
library (`esp_tts_voice_data_xiaole.dat`, header `xiaole_20220719`, 2,938,039
bytes). It is concatenative: recorded syllables stitched together, 16-bit mono
at 16 kHz. It is not a neural text-to-speech model. Espressif's other bundled
voice, xiaoxin, is larger and is not on the robot. English has no syllables in
this set, so an English reply stays on screen at reading pace. The file sits in
a `voice_data` partition at `0x822000`, after the old table, so Wi-Fi and
pairing were not erased. The engine code in the app is about 128 KB. The PC
does not have to stay on, and the reply text is not uploaded to a speech
service. Muse itself still writes the reply in Meta's cloud.

The first build mapped that voice from the chat task. That task's stack is in
PSRAM, and mapping flash turns the cache off, which makes a PSRAM stack
unreachable — the screen flashed and the chip rebooted on every Chinese reply,
with no audio. Mapping now happens on a small internal-RAM stack. A still hold
was also cancelled by any tile-view scroll, including a few pixels of drift,
so only the PWR key still started a recording.

The notes below are why Muse itself is silent and why a PC speech server was
the other option.

**Ignore the marketing on this one.** gadgets.muse.ai says the AiPi Lite
"answers out loud", a widely-shared Medium walkthrough says "Muse answers out
loud" for the Waveshare board, and the SDK's own `devices/esp32-s3-box-3.md`
lists "spoken replies" as a working feature and a test step. All three are
wrong. Verified on commit `693cde9` (2026-10-03) by listing every reference to
the protocol's TTS stream kind:

```
muse_chat_session.cpp:175: enum kind_t : uint8_t { K_NONE, K_SUB, K_DICT, K_CHAT, K_TTS };
muse_chat_session.cpp:181: int msg;   /* K_TTS: index into the turn's messages */
muse_chat_session.cpp:946: if (s.kind == K_TTS) {
muse_chat_session.cpp:1740: case K_TTS:
muse_chat_session.cpp:1771: case K_TTS:
```

Those are the enum, a comment, the reset path, and the two receive handlers.
**No code anywhere opens a `K_TTS` stream.** The receive half is built and
waiting; the request half is yours to write. `Kconfig` and `main/voice.h` state
this plainly — "a TTS API of your own can speak it" — so the source is right
and three documents are wrong.

The heavy lifting does exist: minimp3 decoding, the speaker, volume, caption
sync and backpressure. **Only the MP3 fetch is missing.**

#### Start from chucky1102's branch, not from scratch

[`chucky1102/muse-gadget-sdk @ local-tts`](https://github.com/chucky1102/muse-gadget-sdk/tree/local-tts)
is a **working implementation of exactly this**, posted on Issue #14 and
offered upstream. Behind `CONFIG_MUSE_LOCAL_TTS` the firmware POSTs each reply's
text to an HTTP server you run and plays the MP3 streamed back through the
SDK's existing decoder and speaker path — literally the hook `start_tts()`
describes. Captions still follow the speech. A small `edge-tts` server is
included with its protocol documented at
`esp32/tools/muse/tts_server/README.md`, so swapping in another engine is easy.
**Speech starts a little over a second after the reply text arrives.**

**Take the bug warning with it.** On ESP-IDF 6.0.1 — our exact version —
`vStreamBufferDeleteWithCaps()` can free the buffer twice
([espressif/esp-idf#18855](https://github.com/espressif/esp-idf/issues/18855)),
which shows up as an **occasional reboot at the end of a reply**. Creating the
stream buffer with `xStreamBufferCreateStatic()` avoids it. That is a
miserable, intermittent bug we now simply skip.

Caveat: tested on one board only, a Waveshare ESP32-S3-Touch-AMOLED-2.16, so
expect to port it to CoreS3.

Prior art for the shape of it: `carbeso/stackchan-mcp` does server-side TTS
(POST to VOICEVOX, decode WAV, resample to 16 kHz mono, frame it, push to the
device) and notes "no firmware changes are required: the existing audio decoder
pipeline already accepts these frames". `sodre90/StackChan` does the same with
edge-tts or Piper. Both confirm the sane division of labour — **do the TTS call
off-device** and feed the gadget ready-to-play audio.

The comment inside `start_tts()` in `muse_chat_session.cpp` (~line 1495) spells
out the exact steps: keep `m.tts = TTS_ACTIVE` and `s_turn.tts_msg = i`, set
`s_turn.silent = false`, `m.pcm_start = s_turn.pcm_out`, `m.pcm_frames = 0`,
`s_turn.mp3_len = 0`, `s_turn.mp3_ended = false`, `s_turn.kbps = 0`,
`s_turn.down_rate = 0`, call `mp3dec_init(&s_turn.dec)`, then feed the MP3 to
`tts_data()` as it arrives and set `s_turn.mp3_ended` at the end. `decode()`
does the rest. The reply text is at `s_turn.texts + i * TEXT_MAX`.

Constraints to respect: all network work runs on **one task**, so do the fetch
there; `tts_data()` buffers only up to `MP3_BUF` and silently drops the
overflow, so throttle while full. `esp_http_client` and `esp-tls` are already
dependencies.

Pick a TTS provider that speaks Chinese well and returns MP3 (Meta ships none).
An API key compiled into firmware is as exposed as the SDK token, so prefer a
small TTS proxy on your own LAN.

Ignore `devices/esp32-s3-box-3.md` where it claims "spoken replies play" — that
contradicts both `Kconfig` and the source, which are authoritative.


## Earlier off-device fallback plan

## Fallback plan

If the Muse Gadget SDK path stalls, the alternative keeps wake word and real
two-way speech but gives up the genuine Muse agent: run
[xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server)
locally, repoint the device's NVS `ota_url` at it (no reflash needed), and set
its LLM provider to Meta Model API:

```yaml
LLM:
  MuseSpark:
    type: openai
    base_url: https://api.meta.ai/v1
    model_name: muse-spark-1.3
    api_key: <MODEL_API_KEY from dev.meta.ai>
```

Muse Spark is $1.25/$4.25 per million input/output tokens. Meta ships no TTS,
so that path uses EdgeTTS or similar.

**Status: superseded as the next step.** This fallback was written before the on-chip prototype. The on-chip path exists and is prototype-only. The product still needs a high-quality Chinese and English TTS choice; this section remains as prior art, not as the current plan.

## Why Muse stopped speaking, officially

### Why Muse stopped speaking, officially

[PR #12](https://github.com/facebookincubator/muse-gadget-sdk/pull/12) by
`@anantn` (Meta) explains the whole thing. Muse used to send gadget turns with
`output_modality: "voice"`, but **that voice model is no longer served**, so
every turn failed and the server substituted "Sorry, I ran into a problem while
responding." The TTS endpoint `GET /api/voice/tts-stream` only speaks
voice-modality replies, so it couldn't be reused. The fix switched gadgets to
`output_modality: "text"` and deleted the server TTS fetch, **keeping the MP3
decoder, resampler, speaker playback, caption sync and volume** for your own
TTS.

So the docs promising spoken replies aren't lying, they're **stale** — written
when it worked. Meta's own position, from `@anantn` on
[Issue #14](https://github.com/facebookincubator/muse-gadget-sdk/issues/14):

> We're still working on a robust voice pipeline for gadgets and hope to share
> more on that in the near future! In the short term, I'd recommend a TTS API
> of your choice for spoken responses.

Native voice may come back. Until then, DIY is the sanctioned route.


