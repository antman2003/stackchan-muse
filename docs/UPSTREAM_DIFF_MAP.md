# Upstream diff map

`main` (`6269179`) versus `upstream/main` (`693cde9`).

Two product commits:

- `93a2b7a` working avatar, hold-to-talk, and on-chip speech
- `6269179` product `AGENTS.md`

Nothing here has been merged. This is a map, not a merge plan.

## Added files

| File | Risk | Why |
|---|---|---|
| `AGENTS.md` | LOW | New file. Upstream has no root `AGENTS.md`. |
| `esp32/components/muse/muse_speak.c` | LOW | New file. Behavior is behind `CONFIG_MUSE_BOARD_M5STACK_CORES3`, with stubs otherwise. |
| `esp32/components/muse/muse_speak.h` | LOW | New file. |
| `esp32/components/muse/avatar/muse_pixel.c` | LOW | New file in a directory upstream gitignores. Upstream will not edit it. |
| `esp32/components/muse/avatar/muse_avatar_art.h` | LOW | New art header. Large, but not a textual conflict. |

## Modified upstream files

| File | Risk | Why |
|---|---|---|
| `esp32/components/muse/muse_chat_session.cpp` | HIGH | `start_tts` and `decode` sit in the turn state machine upstream documents and is likely to keep editing. About 40 product lines, including `s_turn.onboard`. |
| `esp32/components/muse/muse_ui.c` | HIGH | About 135 lines of gesture code in a shared UI file that already owns the avatar canvas, captions, and settings. |
| `esp32/components/muse/CMakeLists.txt` | MEDIUM | `muse_speak.c` was inserted into both Hatch source lists, `esp_partition` was added to requires, and a CoreS3 link block was appended. A small upstream edit to those lists conflicts. |
| `esp32/partitions_muse.csv` | MEDIUM | Three lines appended. Any upstream rewrite of the table conflicts. The new row does not move `nvs` or the app slots. |
| `esp32/components/muse/muse_input.c` | LOW | About ten lines. One atomic word, OR'd into the existing poll. |
| `esp32/components/muse/muse_input.h` | LOW | One function declaration. |
| `esp32/.gitignore` | LOW | One line: `sdkconfig.token`. Sits next to the upstream avatar ignore. |

## Not in the diff, but used

These are why the device pairs, joins Wi-Fi, and shows a picture. They are
upstream at `693cde9`:

- `esp32/components/muse/boards/board_m5stack_cores3.c`
- pairing, Wi-Fi, and the LVGL display path
- the avatar switch in `CMakeLists.txt` that prefers `avatar/muse_pixel.c`
- the speaker stream `start_tts` writes PCM into

## Untracked, not in either tree's commit

`esp32/components/muse/muse_font_cjk_20.c` is a local generated font. It is
not referenced by `CMakeLists.txt` or `muse_ui.c`. It is not part of this map's
merge risk until someone adds it.

## Rank

**HIGH.** `muse_chat_session.cpp`, `muse_ui.c`.

**MEDIUM.** `CMakeLists.txt`, `partitions_muse.csv`.

**LOW.** `muse_speak.c`, `muse_speak.h`, both avatar files, `muse_input.c`,
`muse_input.h`, `esp32/.gitignore`, `AGENTS.md`.

## Reference notes

No code was copied from these trees. They are the places to read next.

### Voice, avatar, interaction

Repository: `references/Muse-charm-mosaico` (`samyeei/Muse-charm-mosaico`).

| File | Concept | When |
|---|---|---|
| `projects/muse_companion/main/companion_model.h` | One snapshot of mode (idle, listening, thinking, speaking, error) updated from the UI task. Other tasks send actions. | Later, if face state and speech start fighting across tasks. Not needed while `muse_state` plus `muse_ui` still agree. |
| `projects/muse_companion/main/companion_tts.h` | TTS submit/cancel against a speaker codec handle, with a done callback and a generation id. | Later, if a second utterance must cancel the first. Current code finishes one `muse_speak` utterance inside the chat turn. |
| `projects/muse_companion/main/companion_ui.c` | UI reads the model snapshot instead of owning the gesture and the speech flags. | Later. Relevant to the `muse_ui.c` debt, not to a behavior change now. |

Their companion is a separate ESP-IDF app with its own chat stack. It is
not a drop-in for `muse_chat_session.cpp`.

### StackChan body

Repository: `references/StackChan-BSP` (`m5stack/StackChan-BSP`).

| File | Concept | When |
|---|---|---|
| `src/utils/motion/servo.h` | Servo object with `init`, `update`, and a move-to-angle call. MIT. | When body motion starts. Not used by the current firmware. |

Repository: `references/StackChan` (`m5stack/StackChan`).

| File | Concept | When |
|---|---|---|
| `firmware/main/stackchan/motion/motion.h` | Yaw and pitch servos updated together, with angle limits. | When body motion starts. Read after the BSP header, not instead of it. |

Repository: `references/M5Unified` (`m5stack/M5Unified`).

| File | Concept | When |
|---|---|---|
| `src/M5Unified.inl` | CoreS3 port pin tables (`board_M5StackCoreS3`). | When a new CoreS3 bus is wired. Muse's current display and audio pins stay with the BSP the board file already includes. |
