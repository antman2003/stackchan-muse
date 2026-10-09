# Current architecture

What the firmware does on `main` at `6269179`, compared with official Muse
`upstream/main` at `693cde9`. This describes the code that exists. It does
not describe a future robot layer.

CoreS3 board support, BLE pairing, Wi-Fi, and the display path are upstream
Muse. They are not in the product diff. The product adds a custom avatar,
hold-to-talk on the face, and on-chip Chinese speech.

## What is whose

| Kind | Where |
|---|---|
| Shared Muse code, locally edited | `muse_ui.c`, `muse_input.c`, `muse_input.h`, `muse_chat_session.cpp`, `CMakeLists.txt`, `partitions_muse.csv`, `esp32/.gitignore` |
| Product files in an upstream extension slot | `esp32/components/muse/avatar/muse_pixel.c`, `muse_avatar_art.h` |
| Product files | `muse_speak.c`, `muse_speak.h`, `AGENTS.md` |
| External dependency | sibling `../esp-tts` (headers and the two xiaole libraries). The voice file is flashed into the `voice_data` partition, not linked. |
| Upstream, used as-is | `boards/board_m5stack_cores3.c`, pairing, Wi-Fi, LVGL display, speaker stream |

There is no separate interaction module, motion module, or hardware
abstraction for the StackChan body. Servos, LEDs, and the IMU are not driven.

## Speech path

```
PWR button (board poll_buttons)
or hold on the face (muse_ui touch_hold_tick)
        |
        v
muse_input task
  ORs muse_input_touch_edges() into muse_board->poll_buttons()
        |
        v
existing Muse talk-button / chat session
        |
        v
muse_chat_session.cpp start_tts()
        |
        +-- muse_speak_begin(text) true
        |         |
        |         v
        |   muse_speak.c  (CoreS3 only)
        |   maps voice_data, runs esp-tts xiaole
        |         |
        |         v
        |   speak_onboard() -> muse_speak_take()
        |   PCM into the existing reply stream buffer s_out
        |         |
        |         v
        |   existing Muse speaker path
        |
        +-- muse_speak_begin false
                  |
                  v
            existing silent caption pace
            (English, missing voice, or not CoreS3)
```

`muse_speak_begin` returns false unless the build is CoreS3, the reply
contains a CJK character, and the `voice_data` partition maps. Other boards
compile stub functions that always return false.

Mapping the voice runs on an internal-RAM stack. The comment in
`muse_speak.c` says the chat task stack is in PSRAM, and mapping flash from
that stack resets the chip.

## Avatar path

```
existing muse_state mode
  boot, idle, listening, thinking, speaking, error, off
  plus a pet "happy" amount
        |
        v
muse_ui.c
  builds muse_pose_t
  calls muse_pixel_render() and muse_pixel_scale()
        |
        v
avatar/muse_pixel.c   (present, so CMake uses it)
  warps muse_avatar_art.h
  draws eyes, mouth, and mode effects
        |
        v
existing LVGL canvas
```

Upstream `CMakeLists.txt` already switches to
`components/muse/avatar/muse_pixel.c` when that file exists, and otherwise
uses `esp32/avatar/muse_pixel.c`. The product committed the override. The
UI still talks only to `muse_pixel.h`. Animation is inside that renderer.
There is no second animation module.

Hold-to-talk lives in `muse_ui.c` (`touch_hold_tick`). A still hold injects
talk press/release. A short tap calls `muse_state_make_happy()`. A drag
still opens settings. That logic is in the shared UI file.

## File inventory

### Board / CoreS3

No product diff. `esp32/components/muse/boards/board_m5stack_cores3.c` is
upstream. Display, touch, speaker, and microphones come from that file and
the Espressif CoreS3 BSP.

`muse_speak.c` is compiled for every Hatch build, but the esp-tts libraries
are linked only when `CONFIG_MUSE_BOARD_M5STACK_CORES3` is set.

### UI

| File | Role | Behavior | Shared or product |
|---|---|---|---|
| `esp32/components/muse/muse_ui.c` | Face UI | Hold on the face to talk. Short tap pets. | Shared Muse file, product edit |

Captions, settings, and the avatar canvas around that edit are upstream.

### Avatar

| File | Role | Behavior | Shared or product |
|---|---|---|---|
| `esp32/components/muse/avatar/muse_pixel.c` | Renderer | Implements `muse_pixel.h` with the plush character | Product file in the upstream override slot |
| `esp32/components/muse/avatar/muse_avatar_art.h` | Art | RGB still plus extra poses | Product file |

### Animation

No separate file. Motion is inside `avatar/muse_pixel.c`: warp the still,
crossfade poses, draw the face for the current `muse_pose_t`.

### Input / push-to-talk

| File | Role | Behavior | Shared or product |
|---|---|---|---|
| `esp32/components/muse/muse_input.h` | Hook | Declares `muse_input_touch_edges()` | Shared header, product edit |
| `esp32/components/muse/muse_input.c` | Input task | ORs those edges with the board button poll | Shared file, product edit |

The button state machine below that poll is upstream.

### Speech / TTS

| File | Role | Behavior | Shared or product |
|---|---|---|---|
| `esp32/components/muse/muse_speak.h` | API | `muse_speak_begin`, `muse_speak_take` | Product |
| `esp32/components/muse/muse_speak.c` | Engine | CoreS3 xiaole from `voice_data`; stubs otherwise | Product |
| `esp32/components/muse/muse_chat_session.cpp` | Turn speech | Calls the API from `start_tts` / `decode` | Shared file, product edit |

### Partitions / storage

| File | Role | Behavior | Shared or product |
|---|---|---|---|
| `esp32/partitions_muse.csv` | Flash map | Adds `voice_data` at `0x822000`, 3 MB, after the old table | Shared table, product edit |

`nvs`, `otadata`, and both app slots stay at their upstream offsets.
Pairing and Wi-Fi live in `nvs`.

### Build integration

| File | Role | Behavior | Shared or product |
|---|---|---|---|
| `esp32/components/muse/CMakeLists.txt` | Build | Adds `muse_speak.c`. On CoreS3, links `../esp-tts` | Shared file, product edit |
| `esp32/.gitignore` | Ignore | Ignores `sdkconfig.token` | Shared file, one product line |

### Other

| File | Role | Behavior | Shared or product |
|---|---|---|---|
| `AGENTS.md` | Agent notes | Product overlay. Not firmware. | Product |

Untracked `esp32/components/muse/muse_font_cjk_20.c` is not in the build and
not in this running design.

## Current coupling / technical debt

| File | Coupling | Why it is awkward against upstream | When |
|---|---|---|---|
| `muse_chat_session.cpp` | On-chip speech is a branch inside `start_tts` and `decode`, next to upstream's MP3 instructions | Upstream owns this turn state machine. The next Muse speech change will conflict here | Can wait. Move it only when this function must be edited again |
| `muse_ui.c` | Hold-to-talk and petting are inside the shared face UI | The file is large and changes often upstream. The gesture code is not a board overlay | Can wait. It is the second hotspot, after the chat session |
| `muse_input.c` / `.h` | UI task injects talk edges into the input task | The hook is small, but it is on the shared button poll | Can wait. Keep the hook if the UI stays the gesture owner |
| `CMakeLists.txt` | `muse_speak.c` is on every Hatch source list. esp-tts links only for CoreS3 | A one-line upstream edit to the source lists will conflict. Non-CoreS3 builds still compile the stub | Can wait. The stub keeps other boards building |
| `partitions_muse.csv` | `voice_data` is on the shared full-UI table, not a CoreS3-only table | Any upstream partition edit conflicts. Other Muse boards inherit a 3 MB voice partition they do not use | Can wait until a second board needs a different table |
| `esp32/.gitignore` | One added ignore line, beside upstream's avatar ignore | Low. The avatar directory is still ignored, while two files in it are force-tracked | Leave it |

Direct StackChan body hardware is not embedded in these files. The debt is
product behavior sitting inside shared Muse UI, input, chat, build, and
partition files.
