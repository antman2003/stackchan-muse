# Refactor plan

Smallest next step that still leaves the working CoreS3 firmware alone.

Do not introduce `robot_behavior`, audio, motion, and display layers now.
Those layers do not exist, and nothing in the current code requires them.
The running path is already short: Muse UI and chat call two product hooks
(`muse_input_touch_edges`, `muse_speak_begin` / `muse_speak_take`) and an
avatar file the upstream build already knows how to select.

## Do this first, later, not now

When the next task has to edit `muse_chat_session.cpp` or `muse_ui.c` anyway,
pull the product branch out so the shared file keeps a single call.

1. Leave `start_tts` / `decode` as "ask `muse_speak`". The PCM loop can live
   beside `muse_speak.c` instead of inside the chat session.
2. Leave `muse_ui.c` as "call `muse_touch_talk_tick()`". The hold, tap, and
   pet rules can live in a new product file that still calls
   `muse_input_touch_edges` and `muse_state_make_happy`.

That is the whole first refactor. One call site in each shared file. No new
task, thread, or hardware interface.

Do not do it as a drive-by cleanup. Both paths work, and moving them without
a failing build only creates a merge for no behavior change.

## Files to leave untouched

- `boards/board_m5stack_cores3.c` and the rest of the upstream board, pairing,
  Wi-Fi, and display code
- `avatar/muse_pixel.c` and `muse_avatar_art.h` until the character changes
- `muse_speak.c` internal xiaole mapping, unless speech itself is the task
- `partitions_muse.csv` offsets. `nvs` and the app slots must stay put
- `esp32/AGENTS.md`, `esp32/devices/AGENTS.md`, `linux/AGENTS.md`
- `muse_font_cjk_20.c` until captions are an explicit task

## What should eventually leave shared Muse files

| Shared file | Product piece to move | Trigger |
|---|---|---|
| `muse_chat_session.cpp` | Onboard PCM branch | Next edit to `start_tts` or `decode` |
| `muse_ui.c` | `touch_hold_tick` | Next edit to face gestures |
| `CMakeLists.txt` | CoreS3 esp-tts link, already partly isolated | Only if upstream rewrites the source lists |
| `partitions_muse.csv` | `voice_data` row | Only if upstream changes that table |

`muse_input_touch_edges` can stay. It is the stable seam between the UI task
and the input task.

## What not to refactor yet

- Do not wrap Muse chat, UI, or the speaker in a new behavior layer.
- Do not retarget the avatar onto a new renderer interface.
- Do not vendor `esp-tts` into the repo.
- Do not split `partitions_muse.csv` per board.
- Do not start servo, LED, or IMU code inside `muse_ui.c` or
  `muse_chat_session.cpp`.

## When body hardware starts

Add a new product file that owns the StackChan body. Read, in order,
`references/StackChan-BSP`, `references/StackChan`, and
`references/M5Unified`. Call it from one board or UI site. Do not put pin
numbers or servo writes into shared Muse files.

CoreS3 parts the Muse board file already uses stay on the BSP that file
includes. Do not reclone those vendor trees. The copies under `references/`
are the ones to read.

## Priority

1. Keep pairing, Wi-Fi, display, the custom avatar, its animation, and
   Chinese speech working.
2. Keep product edits out of `muse_chat_session.cpp` and `muse_ui.c` except
   for the single call that remains.
3. Put future servo, LED, and IMU work in a new file.
4. Stop there. A three-layer robot stack can wait until two hardware
   features need the same seam.
