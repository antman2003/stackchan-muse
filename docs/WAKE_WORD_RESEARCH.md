# Wake word research

> Moved from the workspace lab notebook on 2026-10-09.
> Device identifiers (SSID, LAN address, MAC, BLE name, and a personal name in a sample reply) are replaced with placeholders.
> Cross-links that pointed at old README anchors are rewritten below where they would break.

## Status

Hands-free wake is **not implemented**. Touch hold-to-talk is the working input. The product target is a wake phrase, ideally "Hi Muse", that starts the existing Muse conversation path.

Notes below include a corrected partition finding: an earlier version of this research wrongly claimed a new partition required a full erase. The corrected principle still holds: appending a data partition does not move `nvs` or the app slots, so pairing and Wi-Fi can survive. Treat any "must erase the whole flash" claim as **superseded**.

**Stale offset.** The free-tail arithmetic below (7.87 MB starting at `0x822000`, and the example `model` row at `0x822000`) was written before `voice_data` was added. `partitions_muse.csv` now has `voice_data` at `0x822000` size `0x300000`, so that region ends at `0xB22000`. The unused tail is `0x4DE000` bytes (about 4.87 MB), not 7.87 MB. Do not append a model partition at `0x822000`; it would collide with the voice prototype. A later wake partition would have to start at or after `0xB22000`. That placement has not been tested.

**Flash budget needs re-verification.** The "693 KB font" sentence is the uncompressed font-data estimate from `DISPLAY_AND_CJK.md`, not the 4,463,636-byte C source file. That source size is not the flash footprint. The custom avatar grew the app after this note, and the font is still not in the build, so whether a ~693 KB font and ESP-SR both fit in `ota_0` still has to be checked. The linked size of the compressed C font has not been measured with a build.

### 6. Wake word [5] — the biggest job, but no re-pairing needed

The stock StackChan firmware did this fine, so the hardware is not in question.
The factory layout had a 4 MB `assets` partition holding its wake-word model;
that firmware is xiaozhi-esp32, which uses Espressif's **ESP-SR WakeNet**.

It is absent from Muse, not merely undocumented: the device registers itself
with `cJSON_AddBoolToObject(params, "is_wakeup_supported", false)` in
`main/noise_control.cpp`, and no `esp-sr` dependency exists in the SDK.

#### The partition change is additive — pairing survives

Earlier notes here claimed this forces a full erase. **That was wrong.** The
Muse table ends at `0x822000` on a 16 MB chip, leaving **7.87 MB unused**:

```
prod_bak ends   0x822000
flash end       0x1000000
unused tail     0x7DE000 = 7.87 MB
```

ESP-SR only needs a data partition **labelled exactly `model`** (it looks it up
at runtime with `esp_srmodel_init("model")`). Appending it at `0x822000` leaves
`nvs`, `otadata`, `ota_0` and `ota_1` at their current offsets, so **nothing is
wiped and the gadget stays paired**. Only the 3 KB partition table at `0x10000`
is rewritten.

```
model, data, spiffs, 0x822000, 4M,
```

#### Flash budget is fine

The model lives in its own partition, **not** in `ota_0` — only the ESP-SR
*code* lands in the app. So the 693 KB font from step 2 and ESP-SR can coexist
in `ota_0`'s ~2 MB of free space without shrinking anything.

#### A free stock wake word already fits this project

Up to 5 wake words per WakeNet9 model. Notable stock options, free:

| Model | Phrase |
|---|---|
| `wn9_nihaoxiaozhi_tts` / `wn9s_nihaoxiaozhi` | 你好小智 — almost certainly what the factory firmware used |
| `wn9_himfive` | "Hi, M Five" — M5Stack's own |
| `wn9_hiesp`, `wn9_alexa`, `wn9_jarvis_tts`, `wn9_computer_tts` | English |

Start with one of these. A custom "Hi Muse" is the expensive path: Espressif
want 20,000 samples and 2-3 weeks, paid; a third party charges ~$1,000. The
sane alternative is **microWakeWord** — open source, ~50 KB INT8 TFLite models,
trainable in about an hour — but it is ESPHome-oriented, so using it in plain
ESP-IDF means wiring up TFLite Micro yourself. Prove the pipeline with a stock
model before paying for or training anything.

#### Remaining real work

1. **Continuous capture.** The mic is opened on demand for push-to-talk today;
   a wake word needs an always-on loop feeding WakeNet's AFE. Tap in at
   `esp_codec_dev` in `components/muse/boards/board_m5stack_cores3.c`, which
   handles `s_spk`/`s_mic`, `bsp_audio_codec_microphone_init()` and
   `set_mic_gain()` in ES7210 3 dB PGA steps on a shared I2S bus.
2. **Power.** Always-on audio holds a lock that blocks light sleep, so battery
   life drops. WakeNet sits inside the AFE and can be toggled at runtime with
   `afe_handle->disable_wakenet()` / `enable_wakenet()` — use that when asleep
   or on battery.
3. **RAM.** Load exactly one model; esp-sr warns that several WakeNet instances
   at once can exhaust ESP32-S3 internal RAM.
4. **Wire detection to the existing path** by synthesising
   `MUSE_BTN_TALK_PRESS` — the same trick as step 3, so the whole turn state
   machine is reused. Doing step 3 first makes this nearly free.
5. Consider whether flipping `is_wakeup_supported` to `true` changes server
   behaviour. Unresearched; the flag's existence hints the backend understands
   wake-word devices.

#### Flashing the model

`srmodels.bin` is generated by esp-sr's CMake (or manually with
`python <esp-sr>/movemodel.py -d1 <sdkconfig> -d2 <esp-sr> -d3 <build>`), and
its offset appears in `build/flasher_args.json`. It can be written **while the
device is running normally** — no bootloader mode:

```powershell
python -m esptool --chip esp32s3 -p COM3 -b 921600 `
  write-flash 0x822000 build-muse-m5stack-cores3\srmodels\srmodels.bin
```

Flash it **once**. App OTA updates replace only the application image and never
touch the `model` partition, so it only needs rewriting when the esp-sr version
or model selection changes.

Note **step 3 buys most of the same convenience for far less work** — both come
from the same annoyance of reaching for PWR. Do step 3 first, then decide how
much the hands-free part is worth.


