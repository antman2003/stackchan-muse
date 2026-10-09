# Development history

> Moved from the workspace lab notebook on 2026-10-09.
> Device identifiers (SSID, LAN address, MAC, BLE name, and a personal name in a sample reply) are replaced with placeholders.
> Cross-links that pointed at old README anchors are rewritten below where they would break.

This is the chronological lab record: flashing, pairing, debugging, the avatar bring-up, and hold-to-talk. It is not the product roadmap. The product entry point is the workspace `README.md`.

**Status labels used below**

- **prototype-only** — works on the robot, not an accepted product feature
- **superseded** — a later finding replaced this advice
- **needs re-verification** — may have gone stale

Paths in the table below were updated when the workspace was cleaned (`stackchan-muse`, `lab/`, `tools/`, `archive/`). They are current as of that cleanup.

## Where things live

Everything for this robot is in `D:\AI_projects\stack_chan_muse`. The old folder
name `(3)Stack Chan` had a space and parentheses, which break the ESP-IDF
build, so the firmware used to live under `C:\esp`. The compiler install stays
there. This folder name has neither spaces nor parentheses.

| What | Path |
|---|---|
| Muse Gadget SDK (Meta, Apache-2.0) | `.\stackchan-muse` |
| xiaole voice file and esp-tts libraries | `.\esp-tts` |
| Avatar art pipeline | `.\lab\avatar-work` |
| CJK font generator and Noto source | `.\lab\fonts` |
| Serial helpers (`mon.py`, `zhtest.py`) | `.\tools\fw-inspect` |
| ESP-IDF v6.0.1 (the required version) | `C:\esp\esp-idf-v6` |
| ESP-IDF tool downloads | `C:\Users\xiexu\.espressif` |
| Factory flash backup | `.\archive\factory\stackchan-factory-backup-16MB.bin` |
| Prebuilt CoreS3 firmware | `.\archive\firmware\cores3-muse-gadget-firmware-v2-token.zip` |

## Hardware

- M5Stack StackChan, CoreS3 core: ESP32-S3 rev v0.2, 16 MB QSPI flash, 8 MB quad PSRAM
- 2" 320x240 ILI9342C LCD with capacitive touch, AW88298 1 W speaker amp,
  ES7210 dual microphones, AXP2101 PMU, AW9523B IO expander
- MAC `<mac>`
- Serial: **COM3**, native USB Serial/JTAG, VID:PID `303A:1001`
- **PWR** button (left side) is push-to-talk *and* pairing confirmation. There
  is no separate user button. The bottom-edge button resets; hold it 3 s for
  the bootloader.
- **There is no button hold that factory-resets this firmware.** On Muse boards
  the long-press handler is deliberately compiled out (`app.c` ~2606: "Muse's
  talk button confirms pairing and its menu resets setup"), because the Link
  button GPIO can be a display or codec pin. Reset is done from the on-screen
  menu — see [Switch Muse account](#switch-muse-account).


## Factory backup

Taken before any Muse flashing. 16,777,216 bytes,
SHA256 `86547AC590CB2C71A6D40C271BD76C00B6146ECDC84F7085E2B51EE20B9D0B73`.
Verified with two independent full reads that were byte-identical.

Contents: `stack-chan` v1.5.1, built Jul 31 2026, ESP-IDF v5.5.4. The partition
layout (dual OTA plus a 4 MB SPIFFS `assets` partition) is xiaozhi-esp32 v2,
confirming the factory firmware is xiaozhi-derived.

```
nvs       data 0x02 0x009000      16 KB
otadata   data 0x00 0x00D000       8 KB
phy_init  data 0x01 0x00F000       4 KB
ota_0     app  0x10 0x020000   5,056 KB
ota_1     app  0x11 0x510000   5,056 KB
assets    data 0x82 0xA00000   4,096 KB   <- wake word model + face artwork
coredump  data 0x03 0xE00000      64 KB
```

The backup embeds Wi-Fi credentials. Keep it out of version control.

To restore the robot to stock, including Wi-Fi and StackChan World pairing:

```powershell
python -m esptool -p COM3 -b 921600 write-flash 0x0 archive\factory\stackchan-factory-backup-16MB.bin
```

## Flashing the Muse firmware

The prebuilt zip holds `bootloader.bin`, `partition-table.bin`,
`ota_data_initial.bin` and `muse-gadget.bin`, but **no `flash_args`**, so
offsets have to be supplied by hand. Two of them are non-standard:

- **The partition table goes at `0x10000`, NOT the ESP-IDF default `0x8000`.**
  `sdkconfig.defaults` sets `CONFIG_PARTITION_TABLE_OFFSET=0x10000`. Writing it
  to `0x8000` produces a silent boot loop roughly 36 times a second:
  `flash_parts: partition 0 invalid magic number 0xffff` /
  `boot: Failed to verify partition table`. esptool still reports
  "Hash of data verified", because the bytes land fine — just in the wrong place.
  The tell is that `nvs` starts at `0x11000`, immediately after a 0x1000-byte
  table at `0x10000`.
- `otadata` sits at `0x1D000`, not the usual `0xd000`.

Verified build: ESP32-S3, `project_name muse-gadget`, version `999.0.0`,
`idf_ver v6.0.1`, built Oct 4 2026. CoreS3 profile confirmed (only
`"M5Stack CoreS3"` appears, with `ili9342`/`aw88298`/`es7210`/`axp2101`/
`aw9523` drivers compiled in). SDK token embedded.

```powershell
python -m esptool --chip esp32s3 -p COM3 -b 921600 `
  --before default-reset --after hard-reset `
  write-flash --flash-mode dio --flash-size 16MB `
  0x0      bootloader.bin `
  0x10000  partition-table.bin `
  0x1D000  ota_data_initial.bin `
  0x20000  muse-gadget.bin
```

Verified boot log after a correct flash:

```
boot: Loaded app from partition at offset 0x20000
esp_psram: Found 8MB PSRAM device
app_init: Project name:     muse-gadget
link.identity: node_id=homelink-XXXXXX ble_name=MuseGadget-XXXXXX
muse: board: M5Stack CoreS3
muse_pmu: AXP2101 ready (hardware PWR hold-off 10 s)
LVGL: Starting LVGL task
```

This device advertises as **`MuseGadget-XXXXXX`**. Note the prebuilt firmware
reports `NVS encryption not compiled in` / `NVS mode: plaintext`, so Wi-Fi
credentials and the device token sit unencrypted in flash. Enable
`CONFIG_HOMEHUB_NVS_ENCRYPTION` if you rebuild.

To watch the boot log on Windows (`mon.py` resets via RTS first, since
`idf.py monitor` needs the IDF environment):

```powershell
python D:\AI_projects\stack_chan_muse\tools\fw-inspect\mon.py COM3 20
```

Muse partition layout:

```
nvs       data 0x02 0x011000      48 KB
otadata   data 0x00 0x01D000       8 KB
phy_init  data 0x01 0x01F000       4 KB
ota_0     app  0x10 0x020000   4,096 KB
ota_1     app  0x11 0x420000   4,096 KB
prod_data data 0x40 0x820000       4 KB
prod_bak  data 0x41 0x821000       4 KB
```

## Building from source instead

**The toolchain is already installed — do not re-run `install.ps1`.** Verified
Oct 4 2026: all 11 tools under `C:\Users\xiexu\.espressif\tools`, venv
`idf6.0_py3.13_env`, `idf.py` reports ESP-IDF v6.0.1, `xtensa-esp-elf-gcc`
15.2.0, cmake 4.0.3, ninja 1.12.1. Just source `export.ps1` and build. The IDF
version is not optional: Muse requires exactly v6.0.1, which is what the
running firmware reports.

`tools/muse/board.sh build cores3` is the Linux/macOS helper. It is bash and
does not work here. The arg-array pattern below is Meta's documented Windows
equivalent — see `esp32/devices/esp32-s3-box-3.md` in the SDK.

```powershell
. C:\esp\esp-idf-v6\export.ps1
cd D:\AI_projects\stack_chan_muse\stackchan-muse\esp32

$cores3 = @(
    '-B', 'build-muse-m5stack-cores3',
    '-DIDF_TARGET=esp32s3',
    '-DSDKCONFIG=build-muse-m5stack-cores3/sdkconfig',
    '-DSDKCONFIG_DEFAULTS=sdkconfig.defaults;devices/sdkconfig.muse;devices/sdkconfig.muse-m5stack-cores3'
)

idf.py @cores3 menuconfig    # set CONFIG_GADGET_SDK_TOKEN, enable CONFIG_HOMEHUB_NVS_ENCRYPTION
idf.py @cores3 build
idf.py @cores3 -p COM3 flash monitor
```

Boards share `managed_components/` and `dependencies.lock`; never build two
concurrently. After switching boards, delete both if the component manager
complains (lvgl is the usual offender).

## Pairing

Done successfully on 2026-10-04. Keep the robot paired in StackChan World if
you want the factory backup to restore to a paired state — M5Stack's warning
about conflicting pairings concerns *other xiaozhi-based firmware*, not this.

In the Muse app:

1. **Settings > Devices**, and turn on the **Developer mode** toggle. Community
   gadgets are not discoverable until this is on.
2. Tap the **`+` icon in the top right**. There is no menu row labelled "Add
   Device" — the `+` *is* that action. This is the step people get stuck on.
3. Pick `MuseGadget-XXXXXX` (this robot is `MuseGadget-XXXXXX`).
4. Muse warns it is a **community device**; continue.
5. Give it Wi-Fi credentials. **2.4 GHz only** — the ESP32-S3 has no 5 GHz
   radio, so a 5 GHz SSID silently never connects.
6. Press **PWR** to confirm. Meta's docs say BOOT because their reference
   boards use it; the CoreS3 overlay maps confirmation to GPIO 0, which is PWR.

The device must be on a **US** Meta account; the firmware reports
`Region: USA` and the Muse preview is US-only.

Status colours below come from the SDK docs, but **the CoreS3 has no RGB LED**
— the log line `link.led: LED status ready: Muse display` means the status is
drawn *on the screen*. Watch the display, not for a blinking light.

| Status | Meaning |
|---|---|
| Orange, breathing | Ready for setup |
| Blue, breathing | Press the button to confirm pairing |
| Blue | Joining Wi-Fi, connecting to Muse |
| Green | Connected |
| Yellow, blinking | Reconnecting |
| Purple | Not paired |
| Red, blinking | Check the serial log |

Pairing has no manufacturer attestation and cannot stop an active
man-in-the-middle. Set up on a trusted network.

### Checking state without the app

`tools/muse/chat.py` works on native Windows and is the fastest way to see what
the device thinks:

```powershell
cd D:\AI_projects\stack_chan_muse\stackchan-muse\esp32
python tools\muse\chat.py --status          # JSON: board, wifi, pairing, volume
python tools\muse\chat.py "say hello"       # a full chat turn, reply streamed back
```

Non-ASCII arguments get mangled by PowerShell, so for Chinese prompts drive it
from Python instead — see `D:\AI_projects\stack_chan_muse\tools\fw-inspect\zhtest.py`.

The on-device settings UI is reached by **swiping left** on the touchscreen.

## The avatar

**Status: partly superseded.** The robot now runs the custom plush renderer documented under "A better avatar" below. This section is the stock Muse avatar pipeline.

The on-screen avatar is a 64x64 procedural pixel-art renderer with animations
for boot, idle, listening, thinking, speaking, error, off, and a happy reaction
when petted. **Muse Realtime Avatar — Meta's generated-video avatar — has no
public API**, so this renderer is the way to get a Muse face on the screen.

The default avatar is Meta's own character and is explicitly **not** covered by
Apache-2.0. Make your own.

`components/muse/avatar/muse_pixel.c` (gitignored) overrides the default when
present. `tools/muse/avatar.py` asks your Muse to redraw its avatar into that
file, but it needs bash and a host C compiler, so it does not run on native
Windows. The manual path works fine:

1. Paste `tools/muse/avatar_prompt.md` into Muse, attaching `avatar/muse_pixel.c`.
2. Save the returned C file to `components/muse/avatar/muse_pixel.c`.
3. Rebuild. The log prints `Custom avatar: components/muse/avatar/muse_pixel.c`.

To inspect each animation on the device, send `>face=idle` (or `listening`,
`thinking`, `speaking`, `happy`, `error`, `boot`, `off`) on the serial console.
This works in any build.


## Next steps

Ordered easiest to hardest. Numbers in brackets map to the original wish list:
[1] better avatar, [2] readable captions, [3] Chinese, [4] spoken replies,
[5] wake word, [6] switch account, [7] on-screen push-to-talk,
[8] body hardware (servos, camera, sensors, SD).

**Status: superseded as a product scoreboard.** Spoken replies are prototype-only, not a finished feature. The avatar is on the device but is not the final experience. Readable Chinese display is not finished. The original scoreboard was:

**Done:** [6] account switch (phone re-link only), [1] avatar, [7] hold anywhere
to talk (kept instead of an on-screen button, and left this way to try),
[4] spoken replies on the chip (Chinese only, no PC).
**Still open:** [2][3] captions and Chinese, [5] wake word, [8] body hardware.
Step 0 (ask Muse to show a picture) has not been checked on this firmware.

### Step 0: free wins to take before writing any code

**"Images from Muse" already works on this robot.** The SDK's own feature
matrix (`devices/README.md`) marks it ✅ for CoreS3, and
`CONFIG_HOMEHUB_DISPLAY_COMMANDS` is `default y if HOMEHUB_LED_BACKEND_MUSE &&
SPIRAM` — both true here, and our boot log confirms `LED status ready: Muse
display`. So `display.draw_url` is already advertised to the agent in the
firmware we flashed.

This isn't inference — the CoreS3 board PR's test plan ticks "an image from
Muse via `display.draw_url` (320x240 JPEG)" as verified on real hardware.

Ask Muse to show you a picture. It downloads a baseline JPEG and draws it,
hiding the avatar until `display.show_animation`, a tap, or the talk button.
This is the "shows pictures on the color screen" the website advertises, and it
costs nothing.

Also free: the on-device settings UI (swipe left) already exposes volume, mic
gain, brightness, sleep timeout and Wi-Fi, and `tools/muse/chat.py` gives a
full chat turn from the desktop.

### How to execute this wisely

Conclusions after reading gadgets.muse.ai and surveying what others have built:

1. **Read the relevant `AGENTS.md` before each task.** Meta wrote instructions
   for coding agents into every folder — this is the sanctioned workflow, not a
   shortcut. Their own agent is Muse Code; any agent that reads `AGENTS.md`
   works.
2. **Keep every change small and isolated.** Meta is explicit: "The Muse Gadget
   SDK is intended for personal tinkering and is not a supported product or
   developer platform. The SDK can change, break, or stop functioning without
   warning." Treat upstream as a moving target: prefer new files and
   board-overlay config over edits scattered through shared code, so a rebase
   stays cheap. Our font is a good example (one new file, one `srcs` line); a
   hand-patched `muse_ui.c` is the opposite.
3. **Trust source over documentation, including Meta's own.** The TTS claim in
   step 5 appears in Meta's marketing, Meta's own board doc, and a popular
   walkthrough — and is still wrong. Verify against the tree at the commit you
   have.
4. **Reuse community work rather than starting over.** The servo clamps, bus-hang
   behaviour and MIT driver choice below came free from `stackchan-mcp`. Check
   there before solving anything on the base.

**Status: superseded.** An on-chip xiaole prototype was built later; see `VOICE_RESEARCH.md`. That prototype is also not the final speech system. The paragraph below is the original advice.

5. **Do TTS off-device.** Everyone who has shipped working speech on StackChan
   runs the TTS engine on a server and streams audio down. Compiling an API key
   into firmware and parsing a cloud response on the ESP32 is the harder, more
   fragile path.
6. **We are in a better position than most.** CoreS3 is an officially supported
   profile with avatar, touch settings, push-to-talk, images, BLE and tunnel all
   ✅. One developer had to rewrite firmware to port to an unsupported 1.43"
   display, and fought region locks with a VPN and a US iPhone region. We are
   already paired and working — so avoid changes that would risk that state.
7. **The Discord is the place to ask**, linked from gadgets.muse.ai (`#projects`).
   A CoreS3 StackChan profile with servo support is exactly what Meta invites
   people to contribute back.

### Prerequisite for the remaining rebuilds: the SDK token

Steps 2 and 6 still need a **source rebuild**, and the build will not
produce a working gadget without `CONFIG_GADGET_SDK_TOKEN`. Set it through
`idf.py @cores3 menuconfig` and **never commit it** or write it into a board
overlay file. Which token to use is kept outside this file. Step 1 did not
need a rebuild. Steps 3, 4, and 5 are already on the robot.

**Reflashing the app alone preserves pairing and Wi-Fi**, because both live in
`nvs` and the app sits in `ota_0`. A later rebuild costs nothing but the flash
— **no firmware step requires re-pairing**, including the wake word (see step 6
for why its partition change is safely additive).

Step 2 edits `muse_ui.c`, which already contains the hold-to-talk changes, so
keep those when the font goes in.

### 1. Switch to another Muse account [6] — done 2026-10-05, no firmware

Done from the phone: Reset pairing on the robot, then pair again in the Muse
app. No firmware rebuild.

On the robot, **swipe left** to open settings, go to
the **MUSE** page, and tap **"Reset pairing"**. The label changes to "Tap again
to reset"; tap it a second time within 5 seconds. It wipes the app pairing
*and* Wi-Fi, then restarts advertising, so you re-provision Wi-Fi when you pair
the new account. Then pair as above with the other account.

A second route avoids unpairing entirely: the same MUSE page has a **device
token** row, and the SDK notes "a device token here overrides it". Entering the
other account's device token repoints the robot without touching pairing. The
**VM ID** row picks which of that account's VMs to use.

Expect the compiled-in SDK token to keep working — it identifies *you as the
developer*, not the end user. Confirmed 2026-10-05: the robot paired to a
different Muse with no reflash. The `mgst_` token only rides along on
`pairing_confirmed` so the app can mint a device token. Which Muse you talk to
is whichever one the phone app is using when you pair. If the new account is
refused, generate a fresh SDK token from that account and rebuild.

`tools/muse/ble_setup.html`, opened in Chrome, is a third route over Web
Bluetooth, with `wifi.forget` and host/token fields.


### 3. Hold anywhere to talk [7] — on the robot as of 2026-10-06

Shipped as **hold anywhere on the face**, not as another button. A button in
the corner would cover the avatar and is less practical; this mode stays while
it is tried out. Hold still for about half a second to talk; release to send. The settings
page starts to slide at 10 px, which is inside a still hold, and any slide
cancelled the recording, so the slide now waits until the finger has moved
about 36 px.
A quick tap pets (a 220 ms cutoff was stealing ordinary taps, and LVGL drops
the canvas click when the finger jitters, so a short release pets directly).
A drag still opens settings, and the speaker button is left alone. The waking
touch does not start a recording.
Edges go through `muse_input_touch_edges()` into the same path as the PWR
button. Flashed app-only; Wi-Fi and the Muse session came back.

Goal: tap-and-hold the screen to speak instead of reaching for PWR every time.

**The right approach is to synthesise the same button edges the PWR key
produces**, rather than calling the voice API directly. On this board
`poll_buttons()` is just:

```c
unsigned key = muse_pmu_poll_key();
return (key & MUSE_PMU_KEY_PRESS ? MUSE_BTN_TALK_PRESS : 0) |
       (key & MUSE_PMU_KEY_RELEASE ? MUSE_BTN_TALK_RELEASE : 0);
```

OR a UI-sourced edge into that return value and **everything downstream works
unchanged** — `talk_button()` in `muse_input.c`, the wake-marking, the
pairing-confirm swallow, the menu-select case, and `muse_voice`'s recording.
Touching the voice path directly would duplicate that state machine and get the
edge cases wrong.

`build_speaker()` in `muse_ui.c` is a working template for the button itself:
it already registers exactly the event set needed and widens the hit box
(`lv_obj_set_ext_click_area(s_speaker, 12)` — "a fingertip is bigger than the
circle").

```c
static const lv_event_code_t EVENTS[] = { LV_EVENT_PRESSED, LV_EVENT_LONG_PRESSED,
                                          LV_EVENT_SHORT_CLICKED,
                                          LV_EVENT_RELEASED, LV_EVENT_PRESS_LOST };
```

Steps:

1. Add an LVGL button on the face screen; copy `build_speaker()`'s structure.
2. On `LV_EVENT_PRESSED`, latch a pending `MUSE_BTN_TALK_PRESS`; on
   `LV_EVENT_RELEASED` **and `LV_EVENT_PRESS_LOST`**, latch
   `MUSE_BTN_TALK_RELEASE`.
3. Drain that latch inside `board_m5stack_cores3.c`'s `poll_buttons()`, OR-ing
   it with the PMU key edges.

Three gotchas, all real:

- **`LV_EVENT_PRESS_LOST` is mandatory, not optional.** Swiping left opens the
  settings tileview, and a press that becomes a scroll fires `PRESS_LOST`
  instead of `RELEASED`. Miss it and the microphone latches on forever. Note
  `on_speaker_event` deliberately handles `PRESS_LOST` in the same `case` as
  `RELEASED` — copy that.
- **Cross-task handoff.** LVGL events run on the LVGL task; `poll_buttons()`
  runs on the input task. Use an atomic or a short critical section for the
  latch, and accumulate edges rather than overwrite, so a fast tap that lands
  press+release between two polls is not lost. `talk_button()` already copes
  with both edges arriving in one poll.
- **It cannot wake the device.** While asleep, a full-screen `s_cover` overlay
  takes the first touch to wake and deliberately swallows it, "so the tap
  doesn't land on whatever is underneath". So the screen button works only once
  awake; PWR stays the way to wake a sleeping robot.

**Not built: a "hold to talk" button in the lower right.** That was the first
layout, matching other Muse gadget demos so a tap on the avatar could keep
petting. Hold-anywhere replaced it: the same short tap still pets, and the
corner stays clear. The space notes below are only if a button is revisited.

The lower right is genuinely free on this board. CoreS3 sets
`.talk_hint = { LV_ALIGN_TOP_LEFT, 8, 8 }` (the mic icon) and defines no
`aux_hint`, so `build_button_icons()` returns early and draws nothing there.
The speaker button is top-left too. Occupied elsewhere: status line (top
centre), caption band (bottom, full width), navigation dots (bottom centre) —
so align to `LV_ALIGN_BOTTOM_RIGHT` and keep clear of the dots.

Once the button exists, the top-left mic icon is a hint for a button the user
no longer needs to reach for; consider moving it onto the new button or
dropping it.

### 4. A better avatar [1] — done, confirmed on the robot 2026-10-06

**On the robot and working.** The plush avatar is the one in use. More
animation can be added later (further poses beyond the five already blended).
The user's Muse avatar — a cream plush in a red cape — is a high-resolution
renderer that keeps all of the stock avatar's life.
Replaces the procedural fur with the real character photo, warped each frame so
it still breathes, bobs, leans, pops up on boot and hops when petted, with the
eyes, mouth and every state effect drawn on top at the canvas resolution (so
nothing is chunky). Previewed on the PC across all eight states before any
flash; compiles clean under `-Wall -Werror`. Flashed to the CoreS3 on
2026-10-05 with an app-only write (bootloader, partition table, otadata, and
`ota_0`). NVS was not touched, and the boot log showed the saved Wi-Fi and the
Muse session come back. The app is 0x2f1000 bytes, 26% free in the 4 MB slot.

#### How it was made (reproducible)

Work lives in `lab\avatar-work\`:

1. **`make_art.py`** turns the Muse app's avatar picture (`source.webp`, a
   1024x1024 character on a near-white backdrop) into
   `muse_avatar_art.h`:
   - **Matte** by warmth (red minus blue), not brightness — the pale fur shares
     the backdrop's brightness but not its neutrality. A flood-fill from the
     border marks true background; everything the fill can't reach (eyes, mouth)
     is character.
   - **Decontaminate** the fur edge so it doesn't glow when drawn on the black
     screen, by removing the backdrop colour that bled in.
   - **Erase the face.** The eyes and mouth are inpainted out with a
     heat-equation fill, because the renderer draws them itself to animate them.
     The hole is found as the near-black blobs on the upper face (not the cape's
     dark-red folds — an early version grabbed those; the fix was to require
     `max(R,G,B) < 90` and to look for the smile only between and below the
     eyes). Their positions are measured and emitted as `ART_EYE_*`, `ART_MOUTH_*`.
   - **Export** 256x256 RGB565 + 8-bit alpha (192 KB flash) with a
     premultiplied Lanczos resample so edges don't fringe.
2. **`components/muse/avatar/muse_pixel.c`** (gitignored override) is a
   from-scratch renderer that `#include`s the art header and reimplements the
   `muse_pixel.h` interface. The header holds five eye-aligned poses (rest,
   listen, think, happy, wave). Per frame it warps the resting still with a
   pivot-at-the-feet affine (squash/stretch for breathing and boot pop,
   translate for bob/hop, shear for lean) and crossfades in the matching arm
   pose — hands up while listening, a paw on the chin while thinking, both arms
   up when petted, one arm waving while powering off. A partial blend would
   ghost an arm, so each pose eases in fully. Then it draws eyes (blink, gaze,
   wide/happy/X styles), mouth (smile/O/hmm/talk/flat/grin), brows, extra blush,
   and the effects (aura, ground ripples, sparkles, thought dots, hearts,
   error "!") in canvas pixels. Speaking, boot and error keep the arms down.
   All motion constants are copied from the stock renderer so it reacts
   identically. Uses a single RGB565 frame buffer from PSRAM
   (`heap_caps_malloc`, falls back to `malloc` off-device via `ESP_PLATFORM`).
3. **`preview.py`** builds it with the host toolchain and writes a GIF and a
   6-frame filmstrip per state to `previews/`.

#### Host toolchain on Windows (no MSVC, no WSL)

`pip install ziglang` gives a self-contained C compiler: `python -m ziglang cc`.
The SDK's `tools/muse/anim.c` uses POSIX `mkdir(path, mode)`, so a one-line
shim (`win_shim.h`: `#include <direct.h>` + `#define mkdir(a,b) _mkdir(a)`)
passed with `-include` makes it build. This is the whole reason the avatar could
be previewed without flashing, since the SDK's own simulator is Linux/macOS only.

#### Background: why this path, over the alternatives

The 64x64 procedural grid is **fixed by design** — `muse_pixel.h` describes "a
coarse MUSE_PX_W x MUSE_PX_H grid ... blown up with nearest-neighbour blocks so
the pixels stay chunky". The stock renderer is deliberate pixel art. The three
documented routes all stay chunky or need a Linux box:

1. **Replace the character procedurally.** `tools/muse/AVATAR_RECIPE.md` has
   Muse redraw `muse_pixel.c` as a new 64x64 procedural character. Still chunky
   by design, and the user wanted their actual high-res plush.
2. **Fix the fractional scale factor.** `s_canvas_px = s_h * 2 / 3` gives 160,
   and 160/64 = 2.5, so nearest-neighbour makes some blocks 3 px and others 2.
   Not relevant to the photo renderer (it samples smoothly), but worth doing if
   the procedural avatar is ever used: round to whole cells like the other
   paths.
3. **The community avatar studio.** [PR #7](https://github.com/facebookincubator/muse-gadget-sdk/pull/7)
   adds a local browser editor for the avatar: colour, proportion, face
   placement and accessory controls, previewing **the actual firmware
   renderer** across all eight states (boot, idle, listening, thinking,
   speaking, happy, off, error) with pause and scrubbing, and exporting a
   preset, a standalone renderer, GIFs and 64x64 sprite atlases. Runs on
   loopback with Python and Pillow — no board, token or Muse request needed:
   `python3 tools/muse/customize.py`. It was validated on M5Stack StickS3
   hardware with pairing intact.

   Meta **rejected** it — `@anantn` said it was "a bit out of scope of what
   we'd like to see in the SDK repo at this point in time. Feel free to post
   this as a standalone community tool." So it will never merge, but the fork
   `voxeoalyx/muse-gadget-sdk @ feat/avatar-customizer` works. A GUI beats
   hand-editing C for iterating on a character.

`render` must stay under 10 ms on the S3; the photo renderer is a single
warp-and-composite over a ~160 px buffer, well inside that. On the device,
`>face=idle` (also `listening`, `thinking`, `speaking`, `happy`, `error`,
`boot`, `off`) forces a state so each animation can be inspected after flashing.

To change the character later, drop a new picture in as `source.webp` and rerun
`make_art.py` then `preview.py`. To revert to the stock avatar, delete
`components/muse/avatar/muse_pixel.c` (and `muse_avatar_art.h` beside it); the
build picks the default from `avatar/muse_pixel.c` again.

Note `tools/muse/avatar.py` (the automated Muse-redraw flow) calls `board.sh`
(bash), so it won't run on native Windows, and the SDK's desktop simulator is
Linux/macOS only — hence the self-built `preview.py` above.


### Out of scope

**Muse Realtime Avatar** (Meta's generated-video avatar) has **no public API**
and is app/research-only with Video Seal watermarking. The pixel-art renderer
is the only way to get a Muse face on this screen.

## SDK token

Compiled into the firmware binary, so treat it as an identifier rather than a
password. Regenerate at gadgets.muse.ai > Account > SDK tokens. Terms are
personal, non-commercial, 50 devices per token. Do not commit it, paste it
into chat, or put it in a board overlay file. Which token belongs to which
Muse is kept in a local file that git ignores, not in this README.

## What was done, in order

For anyone retracing this (or packaging it as a skill):

1. Researched how to get Muse onto StackChan. Established that **Muse Realtime
   Avatar has no public API**, so the literal goal was impossible, but the
   Gadget SDK's pixel-art avatar — drawn by *your own* Muse — satisfies the
   intent and is officially supported on CoreS3.
2. Chose the Muse Gadget SDK over the xiaozhi-server fallback (kept below).
3. Installed natively on Windows, **no WSL**. Discovered the SDK itself
   documents a PowerShell path, so native Windows is sanctioned, not a hack.
4. Moved all build trees to `C:\esp` to escape the space and parentheses in
   `D:\AI_projects\(3)Stack Chan`. On 2026-10-07 the project trees moved into
   `D:\AI_projects\stack_chan_muse`. The ESP-IDF install stayed at
   `C:\esp\esp-idf-v6`.
5. Took a verified factory backup before anything destructive: two independent
   full reads, byte-identical.
6. Verified the user's prebuilt firmware really was the CoreS3 profile carrying
   their own token, before flashing it.
7. Full `erase-flash`, then flashed. **Hit a boot loop** (see below), fixed it.
8. Paired successfully; confirmed Chinese replies work over `chat.py`.
9. Installed ESP-IDF v6.0.1 and the full toolchain; verified `idf.py` runs.
10. Diagnosed all three display complaints to specific lines of `muse_ui.c`,
    measured the font flash budget, generated a GB2312 font. Stopped here to
    plan rather than continue editing.

## Learnings

### Debugging

- **"Hash of data verified" does not mean the image is usable.** esptool only
  confirms the bytes landed where you asked. Writing a partition table to the
  wrong offset verifies perfectly and still bricks the boot. Read
  `CONFIG_PARTITION_TABLE_OFFSET` from the project, never assume `0x8000`.
- **A partition table does not record its own address**, which is why this class
  of mistake is silent. The giveaway is arithmetic: if the first entry starts at
  `0x11000`, the table is a 0x1000-byte block at `0x10000`.
- **Capture the boot log before theorising.** 798 resets in 22 seconds with zero
  panics immediately said "second-stage bootloader", not "bad app".
- **Read the project's own config before the generic docs.** Both
  `sdkconfig.defaults` and `partitions_muse.csv` state the `0x10000` offset in
  plain comments. Several minutes of debugging would have been a 10-second read.
- **Trust source over prose.** `devices/esp32-s3-box-3.md` claims spoken replies
  work; `Kconfig` and `muse_chat_session.cpp` say Muse does not speak gadget
  replies. The source was right.

### Windows / PowerShell specifics

- `ls -la` is not PowerShell; use `Get-ChildItem`. Avoid naming a helper
  function `Rd` — it collides with the built-in alias for `Remove-Item` and
  produces a baffling "positional parameter" error.
- `Get-ChildItem -Filter` silently returned nothing for a file that existed.
  Confirm with a plain listing before concluding something is missing.
- **PowerShell mangles non-ASCII argv.** Any Chinese string passed to a tool
  gets corrupted. Build the string inside Python and hand it to `subprocess`,
  and write tool output to a UTF-8 file instead of the console (`zhtest.py`).
- Pass many `idf.py` flags as a PowerShell array (`$cores3 = @(...)`, then
  `idf.py @cores3 build`). This is Meta's documented pattern.
- The SDK's bash helpers (`tools/muse/board.sh`, and `avatar.py` which calls it)
  do not run here. The simulator is Linux/macOS only. Prefer the SDK's
  documented "by hand" paths, which are plain Python or `idf.py`.

### Hardware and firmware

- `mon.py` toggles RTS to reset before capturing, which is right for catching a
  boot log but wrong mid-session. `watch.py` opens the port with DTR/RTS
  pre-cleared so it listens **without resetting** — use it while pairing.
- The device's own `>status` console command (via `chat.py --status`) is the
  fastest ground truth for pairing, Wi-Fi, volume and mic gain.
- The prebuilt firmware runs with `NVS mode: plaintext`. Wi-Fi credentials and
  the device token sit unencrypted in flash. Enable
  `CONFIG_HOMEHUB_NVS_ENCRYPTION` when rebuilding.
- The factory backup `.bin` embeds Wi-Fi credentials — never commit it.
- Board classification can silently pick a worse UI: a 320x240 CoreS3 inherits
  the ESP32-S3-BOX-3's compact layout purely because both are `s_w > s_h &&
  s_h < 320`. When a UI looks wrong, check which layout branch the board takes
  before blaming the renderer.
- **Integer scale factors matter for pixel art.** A 2.5x nearest-neighbour blow-up
  makes neighbouring pixel blocks different widths. Always round to whole cells.
- **"Needs a new partition" does not imply "must erase everything."** Check the
  flash tail first: Muse's table ends at `0x822000` on a 16 MB chip, so 7.87 MB
  sits unused and a partition can be *appended* without moving `nvs` or the app
  slots. Only an insertion that shifts existing offsets is destructive. This
  turned the wake word from "wipes your pairing" into a safe additive change.
- **When the user says "but my old firmware did this fine", believe them.** The
  factory StackChan ran wake word on this exact hardware; that made the
  question "how did xiaozhi do it", not "is it possible".
- **Check the tool/command surface, not just the chat protocol.** The gadget
  chat carries only audio and text, which made "Muse can't see images" look
  true. But devices advertise commands to the server with natural-language
  descriptions, and the agent can call them — `camera.capture` returns base64
  JPEG that way. A capability can exist on a side channel.
- **Search for an abstraction before writing a driver.** `components/camera/`
  already defines a board-agnostic `camera_driver_t` with a registration hook
  and a test harness, so adding a camera is filling in a struct, not plumbing
  a new feature end to end.


### Avatar

- **The 64x64 limit is a rendering choice, not a protocol one.** Only four
  functions cross `muse_pixel.h`, and the UI never assumes the pixels came from
  a 64-grid — it just asks `muse_pixel_scale()` for rectangles. So a renderer
  can draw at full canvas resolution and the rest of the firmware is none the
  wiser. The "fixed 64x64 contract" only binds Muse's own art generation, not a
  hand-built renderer.
- **Separate the still from the motion.** A single photo can't give new limb
  poses, but it gives everything else: warping one image delivers breathing,
  bob, squash, lean and hop convincingly, and the face and effects are drawn on
  top. Erasing the face from the still is essential — otherwise the drawn,
  animated face smears over a baked-in one.
- **Matte by the right channel.** Pale fur on an off-white backdrop is nearly
  invisible to a brightness threshold but obvious to warmth (R−B). Picking the
  channel that actually separates foreground from background mattered more than
  any threshold tuning.
- **Detect face features by colour, then by region.** The first face finder
  grabbed the cape's dark-red folds as "eyes". Requiring near-black and then
  constraining the mouth search to the area between and below the eyes fixed it.
- **`pip install ziglang` is a complete, no-admin C toolchain on Windows.** With
  a two-line POSIX `mkdir` shim it builds the SDK's host harness, which is what
  made PC preview possible without MSVC, WSL or the Linux-only simulator.

### Research

- **Documentation lies, including first-party documentation.** Three
  independent sources said Muse speaks its replies aloud: Meta's marketing
  site, Meta's own `devices/esp32-s3-box-3.md` (which lists it as a test step),
  and a widely-read walkthrough. The source tree says otherwise, and the source
  tree is right. Agreement between sources is not evidence when they are
  plausibly copying each other.
- **A five-minute grep beats an hour of reading.** Listing every reference to
  `K_TTS` across the tree settled a question three documents had muddled: the
  handlers exist, nothing ever opens the stream.
- **Check the feature matrix before building a feature.** "Images from Muse"
  was on the wish list implicitly and turned out to be already shipping on
  CoreS3, enabled by a Kconfig default we never had to touch.
- **Search for the hardware, not just the software.** Searching "Muse ESP32"
  found generic builds. Searching for the robot by name found
  `stackchan-mcp` — the same board, the same servos, with its mistakes already
  made and documented.
- **Read other people's issue numbers.** The best details in `stackchan-mcp`
  (the 89° stall sound, the bus hang on reversal) were written up as
  consequences of specific bugs, not as features.
- **Read the project's open pull requests before planning any feature.** This
  was by far the highest-yield half hour of the whole project. Four of seven
  goals already had upstream work, including a merged PR explaining *why*
  speech was removed, a CJK implementation better than my own plan, and a
  working TTS branch. A repo three days old with 209 forks moves faster than
  its documentation.
- **A rejected PR is still useful.** Meta turned down the avatar studio as out
  of scope. The fork still runs, and "won't be merged" is not "doesn't work".
- **Other people's bug reports save days.** The warning that
  `vStreamBufferDeleteWithCaps()` double-frees on ESP-IDF 6.0.1, surfacing as
  a rare reboot at the end of a reply, is the kind of intermittent fault that
  eats a weekend. It cost us one paragraph of reading.
- **Check for known-broken upstream behaviour before trusting your own
  baseline.** Issue #87 reports voice notes returning empty replies on
  ESP32-S3. Without knowing that, a failed voice turn after our changes would
  look like our fault.
- **Stale documentation reads exactly like wrong documentation.** The "spoken
  replies" claims were true once. The giveaway was in version control, not in
  the prose.

### Process

- The user's stated goal and literal request can diverge. "Show the Muse
  avatar" was impossible as asked but achievable in spirit; saying only "no"
  would have been wrong.
- Verify an assumption is still true before acting on it. Earlier advice to
  unpair from StackChan World was wrong, and following it would have made the
  factory backup restore to an unpaired state.
- Check whether a feature is *configurable* before planning to write code. The
  avatar turned out to be a first-class, documented extension point; account
  switching turned out to be two taps in a settings menu.

### The CoreS3 profile's own PR

[PR #25](https://github.com/facebookincubator/muse-gadget-sdk/pull/25) by
`@saint-node` is the board support we are running. Its test plan is the best
available statement of what works on this exact hardware — and it explicitly
confirms **"an image from Muse via `display.draw_url` (320x240 JPEG)"** passed
on a real CoreS3. Also confirmed: PWR press/release edges including a 2.5 s
hold, both mic channels, the speaker, battery over `>status`, pairing, Wi-Fi
and a voice turn captioned. `MUSE_DEFAULT_VOLUME` was added as a Kconfig knob
because the 1 W speaker is quiet, and is 100 here against 70 everywhere else.

Unticked in that plan: **a run on battery, and OTA.** And the PR says plainly
"The camera, proximity sensor, IMU, RTC, SD card and Grove ports aren't used
yet" — which is precisely our step 7, with no one else working on it.

### A live bug that may bite us

[Issue #87](https://github.com/facebookincubator/muse-gadget-sdk/issues/87):
on an ESP32-S3 board, **push-to-talk voice notes return a 0-character assistant
reply** and show the "Sorry, I ran into a problem" fallback, even with PR #12's
text modality confirmed in the binary. Audio uploads fine and transcription
appears correctly in the app; typed turns through `chat.py` work. Meta
acknowledged it on Discord as unexpected and is investigating.

Worth knowing before we blame our own changes for a broken voice turn. Test
push-to-talk on the stock firmware and record the result, so we have a
known-good baseline.

**Status: needs re-verification.** Issue #87 was open when this note was written. Confirm it is still open before relying on it.

## Prior art and community

What already exists, and what each is worth to us.

| Project | What it is | Worth borrowing |
|---|---|---|
| [carbeso/stackchan-mcp](https://github.com/carbeso/stackchan-mcp) | MCP gateway for **this exact 2025 StackChan kit**, CoreS3 + servos + camera, on xiaozhi firmware | The most valuable find. Servo pinout, VM EN power gate, measured pitch limits, bus-hang-on-reversal, MIT vs GPL driver, server-side TTS and STT architecture |
| [sodre90/StackChan](https://github.com/sodre90/StackChan) | m5stack/StackChan fork with a fully self-hosted ASR→LLM→TTS pipeline over WebSocket | Confirms the off-device TTS pattern (edge-tts or Piper, 16 kHz mono). A reference for the fallback plan if Muse proves too limiting |
| [stackchan-arduino](https://github.com/mongonta0716/stackchan-arduino) | Takao Akaki's Arduino servo library, ancestor of the whole scene | SCS0009 positioning timing originates here |
| Kautukkundan's 1.43" AMOLED port | Community port of the Muse SDK to an unsupported display, amplified by Alexandr Wang | Proof that porting Muse to new hardware is tractable. Also a warning: region locks needed a VPN and a US iPhone region |
| [Muse Gadgets Discord](https://discord.gg/3bhjCkZdd6) | Official community, ~205 members online. **Not readable without joining**, so none of the research above comes from it | Where to ask CoreS3 and StackChan questions. Meta staff answer there — Issue #87 was "flagged with the Meta folks on Discord". Worth joining before step 7 |
| [musedirectory.ai setup guide](https://musedirectory.ai/guides/how-to-set-up-a-muse-gadget) | Independent step-by-step | Independently confirms ESP-IDF 6.0.1 exactly, and that reflashing keeps pairing and Wi-Fi |

**Nobody has put Muse on a StackChan yet.** The searches turned up Muse-on-ESP32
builds and StackChan-with-other-AI builds, but no overlap. The two halves exist
separately and we are joining them — which is why the servo, camera and sensor
work has no Muse-side precedent to copy, while the StackChan side is well
trodden.

## References

- SDK: <https://github.com/facebookincubator/muse-gadget-sdk>
- Gadgets site and SDK tokens: <https://gadgets.muse.ai/>
- Meta Model API docs: <https://dev.meta.ai/docs>
- CoreS3 board overlay: `stackchan-muse\esp32\devices\sdkconfig.muse-m5stack-cores3`
- CoreS3 board port: `stackchan-muse\esp32\components\muse\boards\board_m5stack_cores3.c`

