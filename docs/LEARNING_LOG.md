# Engineering learning log

Append only. This is not the status tracker.

If later evidence overturns an entry, add a new entry titled `CORRECTION TO: <date and topic>`. Do not silently rewrite the old one.

## 2026-10-10 — Muse / CoreS3 baseline shape

Evidence level: SOURCE-CONFIRMED

### Question

What does the running product actually wire together?

### What I learned

Muse chat on the CoreS3 drives the display, touch hold-to-talk, and the xiaole speech hook. There is no interaction layer, motion layer, or hardware-abstraction layer beside that. The confirmed bring-up is boot, pairing, Wi-Fi, display, the custom avatar, hold-to-talk, and the speech prototype.

### Evidence

`CURRENT_ARCHITECTURE.md`. The behavior checklist is the project baseline, not a new measurement from this note.

### Engineering intuition

New work should land next to the file that already owns the behavior until a second system needs the same state.

### Why it matters to this project

Phase 0 exists to keep this path reproducible. Later phases must not assume a framework that is not in the tree.

### Open questions

None for the shape of today's path.

### Related milestone / decision

ADR-001, ADR-004. Phase 0.

## 2026-10-10 — Upstream avatar override

Evidence level: SOURCE-CONFIRMED

### Question

How does the custom avatar replace the stock face without forking the renderer call?

### What I learned

The Muse UI still calls the pixel render and scale entry points. Upstream CMake uses `components/muse/avatar/muse_pixel.c` when that file is present, and the stock renderer otherwise. The product avatar files were force-added because that directory is gitignored.

### Evidence

`CURRENT_ARCHITECTURE.md` and the avatar section of `DEVELOPMENT_HISTORY.md`.

### Engineering intuition

A present override file is the extension point. Dropping a new file into that ignored directory does not put it in Git unless it is force-added.

### Why it matters to this project

Avatar improvements should keep that override. Losing the two force-added files silently returns the stock face.

### Open questions

None.

### Related milestone / decision

Phase 2. The current face is PROTOTYPE relative to the final avatar, even though the override mechanism works.

## 2026-10-10 — Xiaole is concatenative and prototype-only

Evidence level: SOURCE-CONFIRMED

### Question

Is on-device xiaole the product voice?

### What I learned

Xiaole is concatenative Chinese syllable speech from esp-tts, mapped from the `voice_data` partition, and hooked into the Muse chat turn. English has no syllables in that set, so English stays on screen. The approved product classification is PROTOTYPE: the voice is mechanical, English is not spoken, and replies can take more than 10 seconds. That 10-second figure is the stated product problem. It has not been split into T0–T7.

### Evidence

`VOICE_RESEARCH.md`. `partitions_muse.csv` places `voice_data` at `0x822000` size `0x300000`.

### Engineering intuition

Do not optimize or extend xiaole until a measurement says where the time goes, and do not treat it as the final TTS.

### Why it matters to this project

Phase 1 and the current sprint exist to measure this path and then replace it if the evaluation says so.

### Open questions

Which T0–T7 gap dominates. Which architecture replaces this path.

### Related milestone / decision

ADR-003. M1 has not started.

## 2026-10-10 — CJK font source size is not the flash size

Evidence level: SOURCE-CONFIRMED

### Question

Is `muse_font_cjk_20.c` a 4.46 MB flash cost or a 693 KB font?

### What I learned

The file on disk is 4,463,636 bytes of generated C source. The research notes' 693 KB figure is the uncompressed binary estimate (`--format bin --no-compress`, 2 bpp, GB2312 subset). Those are different quantities. The C array that would be linked is compressed (`bitmap_format = 1`); its `glyph_bitmap` is 537,954 bytes, plus descriptors and maps. The font is not in the build. The linked size has not been measured with a build. The `.bin` probe is not in the tree, so 693 KB was not recomputed.

### Evidence

`DISPLAY_AND_CJK.md`. Byte counts taken from the generated file, not from a firmware image.

### Engineering intuition

Plan flash from the binary or the linked size. Do not plan it from the `.c` file length.

### Why it matters to this project

M4 cannot assume the font fits, and it also cannot reject the font for being 4.46 MB of source.

### Open questions

Linked size in `ota_0` after the custom avatar.

### Related milestone / decision

M4. Not started.

## 2026-10-10 — StackChan servos are serial, not PWM

Evidence level: SOURCE-CONFIRMED

### Question

Can body motion follow a typical SG90 PWM tutorial?

### What I learned

The body research says the official kit uses SCS0009 feedback servos on a shared UART (G6 TX / G7 RX). StackChan config uses `servo_type: "M5_SCS"`. PWM SG90 Grove tutorials do not apply. This firmware does not drive the servos. Feetech's own library is a license problem; the research points at an MIT driver instead. Published angle limits and the "no abrupt reversal" note are design constraints from that research, not measurements on this robot.

### Evidence

`BODY_HARDWARE_RESEARCH.md`, section "Servos are serial bus, not PWM".

### Engineering intuition

Any motion work starts by reading StackChan-BSP, then a neutral-and-return test inside the published range. Do not copy a PWM example.

### Why it matters to this project

Phase 4's safe motion API has to speak the serial servo protocol. Voice control and speaker turns both depend on it.

### Open questions

Bench confirmation of limits, IDs, and supply enable. Not in scope until Conversation UX Foundation is acceptable.

### Related milestone / decision

ADR-008. Phase 4. Not started.

## 2026-10-10 — No shared character state in the firmware

Evidence level: SOURCE-CONFIRMED

### Question

Does a character-state object already coordinate avatar, text, voice, and body?

### What I learned

No. Face modes exist inside the current avatar path (boot, idle, listening, thinking, speaking, happy, error, off). Speech and captions are hooked from the chat turn. Servos, IMU reactions, and LEDs are not driven from a shared state. The future set adds `working` and is a product target, not a type in the tree.

### Evidence

`CURRENT_ARCHITECTURE.md` avatar path. `PRODUCT_PLAN.md` Phase 6 states the target and says it is not implemented.

### Engineering intuition

Do not add the type until avatar, text, voice, and body all need the same state.

### Why it matters to this project

M7 may show listening, thinking, and speaking on one turn without creating that layer. Phase 6 is the convergence point.

### Related milestone / decision

ADR-004, ADR-006.

## 2026-10-10 — Wake and the conversation session are different

Evidence level: DECISION

### Question

Does continuous conversation wait for "Hi Muse"?

### What I learned

No. Wake is how a session starts. The session is the listening / thinking / speaking / listening loop after that, ended by timeout, stop, sleep, power policy, or the user leaving. A manual start may open the session before a wake phrase exists. Today's path is still one press per turn.

### Evidence

Approved product direction in `PRODUCT_PLAN.md` Phase 3. Not implemented.

### Engineering intuition

Build the session loop against a button before spending the always-on microphone budget.

### Why it matters to this project

Phase 3 is a major feature, and it is not the current sprint.

### Open questions

What object owns the session. How playback is kept from retriggering the microphone.

### Related milestone / decision

ADR-005.

## 2026-10-10 — Speaker orientation is demonstrated, not ported

Evidence level: DEMO-CONFIRMED

### Question

Is turning toward the speaker an unproven idea?

### What I learned

A StackChan + Muse demo already does it, so the capability is a confirmed product goal. This tree does not contain the port. The reference implementation, estimator, microphone geometry, confidence metric, and yaw mapping are not identified. Those details must come from reading that source. Direction must become a filtered yaw target through the safe motion API, not a raw servo write.

### Evidence

Product decision recorded from the approved demo. No source file in this workspace has been named for the estimator.

### Engineering intuition

Treat the behavior as real and the implementation as unknown. Do not sketch a DOA pipeline from memory.

### Why it matters to this project

Phase 5 is a port. It waits until conversation UX is acceptable and until Phase 4 has a safe yaw target.

### Open questions

Which demo source to read first.

### Related milestone / decision

ADR-007. Phase 5. Not started.

## 2026-10-10 — State semantics are not a state framework

Evidence level: DECISION

### Question

Does naming listening, thinking, working, and speaking require a central character-state object?

### What I learned

No. State semantics early; shared state infrastructure late. A phase can be known locally in the code that owns a turn. A shared object is justified when avatar, text, TTS, motion, and LEDs all need that same phase. Building the framework first couples those systems before they exist.

### Evidence

AF-001, accepted as ADR-010. No such object is in the firmware.

### Engineering intuition

A semantic vocabulary is a contract. A central bus is a dependency. Do not add the dependency to hold the contract.

### Why it matters to this project

Phase 1 can talk about thinking and speaking without Phase 6's shared layer. Phase 6 still does not start early.

### Open questions

None for this distinction.

### Related milestone / decision

ADR-010. M1 has not started.

## 2026-10-10 — Character state is not the activity

Evidence level: DECISION

### Question

Should dance, nod, and orient-to-speaker be character states?

### What I learned

No. Character state is the interaction phase. Behavior is what the robot is doing. They combine: happy while dancing, listening while turning toward a speaker, speaking while nodding, working while a capture runs. Putting each activity in the state enum makes mutually exclusive phases out of overlapping work.

### Evidence

AF-004, accepted as ADR-013.

### Engineering intuition

If two things can be true at once, they do not belong in one enum.

### Why it matters to this project

Phase 7 dance and Phase 5 head turns stay activities. They do not grow the state list, and they do not require a behavior manager yet.

### Open questions

None until two implemented activities actually conflict.

### Related milestone / decision

ADR-013.

## 2026-10-10 — On-demand vision is not an always-on perception loop

Evidence level: DECISION

### Question

Does "Muse can look" mean the camera runs continuously?

### What I learned

No. The approved camera tool is a bounded capture: Muse asks, one image or a short bounded capture is taken, the model uses it, and the conversation goes on. That can sit inside working. Continuous streaming, always-on detection, and autonomous scene behavior are a different system, with their own compute, memory, power, bandwidth, latency, privacy, and architecture costs. They are out of scope.

### Evidence

AF-003, accepted as ADR-012.

### Engineering intuition

A tool call has a start and an end. A perception loop does not. The product asked for the tool call.

### Why it matters to this project

Phase 5 camera work, when it starts, is `camera.capture` or an equivalent bounded inspection. It is not a vision runtime.

### Open questions

None until a product goal cannot be met with a bounded capture.

### Related milestone / decision

ADR-012. Phase 5. Not started.

## 2026-10-10 — A continuous session can be a series of bounded turns

Evidence level: DECISION

### Question

Does continuous conversation require a microphone that stays open to the Muse cloud?

### What I learned

No. A session can record one bounded utterance, submit one Muse turn, play the reply, and listen again. The user does not press for each follow-up, so the UX is continuous. Permanent realtime streaming, full-duplex cloud audio, and an always-open cloud microphone are not current requirements. What caps one voice note today is unknown. It might be a buffer, a timer, the protocol, the UI, or the cloud. A number such as 8 seconds is not a confirmed limit.

### Evidence

AF-005, accepted as ADR-014. The duration limit was not read out of source for this note.

### Engineering intuition

Continuity is who starts the next listen. It is not the same as keeping a socket full of audio.

### Why it matters to this project

Phase 3 can aim at a session of voice notes. Phase 1 still measures one turn.

### Open questions

What currently limits a single Muse voice-note duration?

### Related milestone / decision

ADR-014. Phase 3. Not started.

## 2026-10-10 — Barge-in is a local playback problem

Evidence level: DECISION

### Question

Does interrupting speech require streaming audio to the cloud the whole time the robot talks?

### What I learned

The required behavior is local. While TTS plays, something has to notice real user speech, stop that playback, and take a new utterance. The hard part is telling the user from the robot's own speaker. AEC and playback-aware detection are the likely tools. Whether this CoreS3 path can hear and play at once, and whether a future TTS path can cancel quickly, are open. None of that is supported today.

### Evidence

AF-007, accepted as ADR-016.

### Engineering intuition

Barge-in fails in the room, at the microphone, before it fails in the cloud.

### Why it matters to this project

Phase 3 owns this target. It is not part of making one turn fast.

### Open questions

Can the CoreS3 detect user speech during TTS? What AEC or playback-aware detection exists? Can TTS cancel quickly enough?

### Related milestone / decision

ADR-016. Phase 3. Not started.

## 2026-10-10 — Session state is not character state

Evidence level: DECISION

### Question

Does `working` mean the conversation is closed until the task finishes?

### What I learned

No. Session active or inactive is one dimension. Listening, thinking, working, and speaking are another. An active session can be in any of those phases. The product keeps the option to stay receptive during working. That does not add a second state machine to the firmware.

### Evidence

AF-006, accepted as ADR-015.

### Engineering intuition

If the user can still be in the conversation while a task runs, the conversation flag and the task flag are not the same bit.

### Why it matters to this project

Phase 3 can keep the session open across thinking and speaking without deciding, on day one, that working locks the microphone.

### Open questions

Whether an early build must suspend listening during working anyway. ADR-017 allows that.

### Related milestone / decision

ADR-015.

## 2026-10-10 — Hearing "stop" is not the same as undoing the action

Evidence level: DECISION

### Question

If the session notices an interruption, has the email, the motion, or the capture been cancelled?

### What I learned

No. The session can request a cancel. The action decides. Playback usually stops. A servo usually stops through the safe motion API. A capture might be dropped. A network call depends on the tool. A sent email can already be irreversible. Useful future labels are interruptible, cancelable, committed, and completed. They are not a module.

### Evidence

Accepted with AF-008 as part of ADR-014 and ADR-017.

### Engineering intuition

Detection is an input event. Commit is a side effect. They meet only if the tool is still willing to stop.

### Why it matters to this project

Barge-in can stop speech without pretending every tool can be rolled back.

### Open questions

Which future tools become non-cancelable, and at what moment.

### Related milestone / decision

ADR-014, ADR-017. No action framework.

## 2026-10-11 — CORRECTION TO: 2026-10-10 — A continuous session can be a series of bounded turns

Evidence level: SOURCE-CONFIRMED

### Question

What currently limits a single Muse voice-note duration?

### What I learned

Not an 8-second timer. `muse_voice.c` stops hold-to-talk at release or at `MAX_SECS` (15). The 8-second figure in `muse_chat_session.cpp` is `IN_BYTES` (`MIC_RATE * 2 * 8`), the mic backlog while connecting. Filling it can drop live queued audio. It does not end the recording. The client will also accept 20 seconds (`NOTE_MAX_BYTES`); this recorder never reaches that. A comment says the server stops at 15 seconds. That server limit is still UNKNOWN. The connected path streams chunks during the hold and sends the last chunk after release.

### Evidence

`VOICE_RESEARCH.md`, section "Current voice-note and barge-in source findings". The 2026-10-10 entry left this as an open question. This entry closes that question. It does not rewrite the older entry.

### Engineering intuition

A constant that numerically corresponds to N seconds may represent queue depth, not a UX timeout. Trace what happens on overflow before treating it as the interaction length.

### Why it matters to this project

Do not plan conversation length around 8 seconds. The local cap today is about 15 seconds, and that number is an implementation fact, not a product requirement.

### Open questions

Whether the server independently truncates a note, and at what length, if that ever matters.

### Related milestone / decision

ADR-014. Phase 3. Not started.

## 2026-10-11 — Buffer capacity is not the same as interaction duration

Evidence level: SOURCE-CONFIRMED

### Question

If a buffer is sized as N seconds of audio, is the utterance limited to N seconds?

### What I learned

No. `IN_BYTES` holds 8 seconds of 16 kHz mono s16 (256000 bytes). Overflow drops samples from the live queue and logs `mic backlog full`. `record` keeps going until release or 15 seconds, and the kept PCM can still be sent. Queue depth and the hold length are different controls.

### Evidence

`muse_chat_session.cpp` `IN_BYTES` and `muse_hatch_turn_audio`. `muse_voice.c` `MAX_SECS`.

### Engineering intuition

A constant that numerically corresponds to N seconds may represent queue depth, not a UX timeout. Always trace overflow behavior before interpreting it.

### Why it matters to this project

An observed short note is not evidence of an 8-second product limit unless a log shows the recorder itself stopped.

### Open questions

None for the meaning of `IN_BYTES`.

### Related milestone / decision

ADR-014.

## 2026-10-11 — Continuous conversation can reuse streamed bounded turns

Evidence level: SOURCE-CONFIRMED

### Question

Does a continuous session require a microphone that stays open to the cloud for the whole conversation?

### What I learned

No. The current Hatch turn already opens the note at press, sends PCM chunks while the user holds, and sends the final chunk after release. A session can start the next listen after the reply without changing that turn shape. Permanent realtime cloud audio is not required for that UX.

### Evidence

`muse_chat_session.cpp` `open_note` and `record_note`. ADR-014. `VOICE_RESEARCH.md`.

### Engineering intuition

A continuous UX does not require a permanently open cloud audio stream. Session continuity can sit above a bounded/streaming turn protocol.

### Why it matters to this project

Phase 3 can add who starts the next listen without redesigning the upload.

### Open questions

None for the upload model. Session policy itself is still future work.

### Related milestone / decision

ADR-014. Phase 3. Not started.

## 2026-10-11 — Barge-in is mostly local duplex and audio-front-end coordination

Evidence level: SOURCE-CONFIRMED

### Question

Is the Muse protocol what blocks speaking barge-in?

### What I learned

No. The cloud turn can stay a bounded streamed note. The conversation loop writes the speaker and does not read the microphone during playback. The product path has no AEC and no VAD. A press during playback already cancels the software queue. Voice barge-in still needs capture during playback, a way to tell the user from the speaker, and a faster stop of audio that is already queued.

### Evidence

`muse_voice.c` `hatch_reply`. `VOICE_RESEARCH.md`. ESP-SR AFE with AEC and VAD is reference-only in `references/xiaozhi-esp32/main/audio/engines/afe_audio_engine.cc`.

### Engineering intuition

The cloud protocol is not the primary blocker. The hard parts are simultaneous capture and playback, echo suppression, user-speech detection, and fast output cancellation.

### Why it matters to this project

Phase 3 barge-in is local audio work. It is not a second cloud streaming design.

### Open questions

Whether CoreS3 capture is reliable while the speaker runs. Which AEC or VAD integration to use. How completely playback can be flushed.

### Related milestone / decision

ADR-016. Phase 3. Not started.

## 2026-10-11 — Software queue cancellation is not necessarily physical audio cancellation

Evidence level: SOURCE-CONFIRMED

### Question

Does draining the playback buffer mean the speaker goes silent immediately?

### What I learned

Not by itself. `muse_hatch_turn_cancel` empties `s_out` and the voice task stops writing. There is no `muse_speak_cancel` and no codec or DMA flush. Samples already handed to the DMA can still be heard. The next recording throws away about 200 ms of microphone so that tail is not the next utterance.

### Evidence

`muse_hatch_turn_cancel`, `drain_out`, and `SETTLE_CHUNKS` in `muse_voice.c`. `VOICE_RESEARCH.md`.

### Engineering intuition

Draining application PCM does not guarantee already queued codec or DMA audio is gone.

### Why it matters to this project

A barge-in that only clears `s_out` can still talk over the first syllables of the user's next sentence.

### Open questions

Whether an explicit codec close, abort, or silence flush can cut that tail, and how long the tail is on this board.

### Related milestone / decision

ADR-016. Phase 3. Not started.

## 2026-10-11 — M1 one-turn latency trace

Evidence level: MEASURED

### Question

Where does one current Chinese hold-to-talk turn spend time from release to the first playback write?

### What I learned

One turn, `turn=1`, status `complete`. The note was 6.74 s. Deltas are milliseconds after T0.

| Mark | Hook | ms after T0 |
|---|---|---|
| T0 `end_speech` | `muse_voice.c` `record`, on `MUSE_PTT_UP` | 0.0 |
| T1 `final_input_submitted` | `muse_chat_session.cpp` `record_note`, after `send_note_part(true)` | 149.5 |
| T2 `first_response` | `on_event`, first assistant `delta.text_append` | 34096.9 |
| T3 `reply_text_complete` | `on_event`, first `delta.message_done` | 35157.6 |
| T4 `tts_requested` | `start_tts`, immediately before `muse_speak_begin` | 35161.9 |
| T5 `first_pcm` | `speak_onboard`, first `muse_speak_take` with samples | 35251.4 |
| T6 `playback_submit` | `muse_voice.c` `hatch_reply`, before the first non-silent `muse_audio_write` | 35252.7 |
| T7_PROXY `playback_write_done` | same write returning `ESP_OK` | 35253.1 |

T1 is the final voice-note body close after release. `open_note` at press is earlier and is not T1. The tail after release was logged separately at +120.1 ms and is not T0.

`T7_PROXY` is the codec accepting the first non-silent buffer. It is not measured speaker onset.

Gaps: T0–T1 149.5, T1–T2 33947.3, T2–T3 1060.7, T3–T4 4.3, T4–T5 89.4, T5–T6 1.2, T6–T7_PROXY 0.3.

On this single trace, almost all of the time is T1–T2, waiting for the first assistant text. Local TTS and playback handoff after the text is complete are under 100 ms. That is one turn, not a tuning result.

### Evidence

Serial log from the CoreS3 after an app-only flash at `0x20000`. Wi-Fi stayed up and the existing Muse session answered. Xiaole parsed and spoke the Chinese reply. A short tap was not part of this test.

### Engineering intuition

The clock that matters for this path starts when the finger lifts, and the long wait starts after the note is closed.

### Why it matters to this project

M2 can record more turns against this same trace shape. Nothing here says which server or network stage fills T1–T2.

### Open questions

What inside T1–T2 is upload, server wait, or first-token delay. True audible onset versus `T7_PROXY`.

### Related milestone / decision

M1 passed. M2 not started.

## 2026-10-11 — M2 baseline latency set

Evidence level: MEASURED

### Question

Is the M1 gap of about 33.9 seconds from final upload to first reply text the usual case?

### What I learned

Five complete Chinese turns and one English turn, same robot, saved Wi-Fi, existing Muse session. Times are milliseconds after T0. Total is T0 to `T7_PROXY` for spoken turns.

| Turn | Lang | Input s | T0–T1 | T1–T2 | T2–T3 | T3–T4 | T4–T5 | T5–T6 | Total |
|---|---|---|---|---|---|---|---|---|---|
| C1 | zh | 2.86 | 827.0 | 25421.8 | 846.3 | 3.8 | 43.0 | 1.5 | 27144.1 |
| C2 | zh | 3.04 | 162.0 | 18568.7 | 724.7 | 1.1 | 24.5 | 1.2 | 19482.9 |
| C3 | zh | 2.66 | 164.2 | 17478.7 | 1058.4 | 0.7 | 29.6 | 1.2 | 18733.6 |
| C4 | zh | 2.62 | 155.1 | 18735.2 | 1036.1 | 3.7 | 27.9 | 1.2 | 19959.9 |
| C5 | zh | 2.92 | 137.2 | 14736.7 | 557.5 | 4.3 | 28.4 | 1.2 | 15466.0 |
| E1 | en | 4.24 | 176.3 | 12860.6 | 408.0 | 4.2 | — | — | — |

Chinese T1–T2: min 14736.7, median 18568.7, max 25421.8. Mean 18988.2.

Chinese total: min 15466.0, median 19482.9, max 27144.1. Mean 20157.3.

The M1 value of 33947 ms is higher than every Chinese T1–T2 in this set. The gap is still the dominant one on every turn, including English, and it stays in a band of about 13 to 25 seconds here. It is not a single first-turn accident, and it is not gone after warm-up.

C1 was the slowest Chinese turn. The log shows a second VM connect during that press, and T0–T1 was 827 ms instead of about 140–180 ms. C5 was the first turn after another boot and had the fastest Chinese T1–T2, with only the boot-time connect. A first turn is not always the slowest.

English was shown, not spoken. The log says `showing message` and the summary is `ended`. T5, T6, and `T7_PROXY` are absent. T4 was still emitted because the trace marks the call immediately before `muse_speak_begin`, and that call returned without starting xiaole. Those three playback marks were not invented.

Two other presses are excluded. One Chinese note reached T4 and then the log shows `RTC_SW_CPU_RST` before any PCM. One later note was still waiting when a new serial capture reset the USB port. Neither is in the five.

### Evidence

Serial captures after the M1 app-only flash. Wi-Fi came back after each reset and the Muse session connected. Chinese turns log `speaking message` and xiaole playback. After the English summary, heartbeats continue and there is no new press-to-talk. The short tap does not write a log line. The tap did not start a recording. The face change itself is not in the serial log.

### Engineering intuition

The long wait is after the note is closed and before the first reply text. Speech synthesis is not where this baseline spends its time.

### Why it matters to this project

M3 can compare a new speech path against a baseline whose dominant gap is T1–T2, not T4–T5. One trace at 33.9 seconds overstated the center of this set and understated how steady the multi-second wait is.

### Open questions

What inside T1–T2 is still on the device, and what is the server. Why the CPU reset happened after T4 on the excluded turn.

### Related milestone / decision

M2 passed. M3 not started.

## 2026-10-12 — M2 follow-up, residual first-text latency

Evidence level: MEASURED. Transcription cost: UNKNOWN on every turn. A model change in the middle is USER-OBSERVED; the device log does not name a model.

### Question

After the server-side latency change, how much T1–T2 remains, and does that remainder have fast and slow modes, a session-age trend, or a reconnect penalty?

### Setup

Same firmware and M1 marks. Same short Chinese prompt, “一加一等于几？”, hold-to-talk, about 1.8–4.8 s of speech. No intentional reboot. Opening the serial port reset the chip once before the first turn. A later double exception during xiaole parsing rebooted the chip again. The English control was not collected. No firmware, protocol, TTS, or UI change.

The voice-note path does not log a transcript or a transcription-cost class. There is no `heard:` line in this capture. Residual latency was not computed. Turns that need a class are marked UNKNOWN.

### What I learned

Twelve Chinese turns have a complete T1–T2. One press is excluded: it reached T2 about 10.1 s after T1, then panicked during speech parsing (`RTC_SW_CPU_RST`) before a summary. Three later presses arrived after this set was closed. All were spoken, with no reconnect, and none are in the statistics below. Their speech and T1–T2 times were 2.56 s / 7.929 s, 3.74 s / 10.182 s, and 2.50 s / 3.699 s.

One turn reconnected during the press (connect about 1.8 s). It is not in the warm-session numbers. Its T1–T2 was 15.519 s.

Warm-session T1–T2, n=11, seconds: min 2.437, median 6.280, max 44.804, mean 10.168, sample standard deviation 12.127. The mean and deviation are pulled by the 44.804 s turn. No-transcription count is 0, so that subset has no min, median, max, mean, or deviation.

T1–T2 at or below 3 s: 2 of 11 warm turns. Above 5 s: 7 of 11. The warm values run 2.4, 2.8, 3.1, 4.6, 5.7, 6.3, 7.3, 7.5, 12.8, 14.5, then 44.8 s. No empty gap separates a fast mode from a slow mode.

Order does not show a steady session-age slope. The first turn of the first boot was 14.5 s and the next was 6.3 s. After the crash, the reconnect turn was 15.5 s, then 12.8 s, then 44.8 s. Later turns were mixed.

Six turns in the middle showed 2–4 characters and did not speak. Their T1–T2 values were 2.437, 7.454, 3.135, 4.616, 5.684, and 2.795 s (median 3.875, mean 4.353). The turns before and after that stretch spoke longer replies, including a 242-character reply at 7.309 s after the short stretch. Shorter answers match a quicker spoken turn only because xiaole did not run. They do not pin first-text time to one value.

### Evidence

Serial capture on the existing M1 firmware. Summaries `complete` when xiaole spoke, `ended` when the reply was shown and not spoken. Reconnect is the one `connected to VM` line during a press. No transcription event was logged.

### Engineering intuition

Same prompt, same client, and a closed voice note still leave a multi-second spread before the first text. That spread is consistent with serving, queue, or model time-to-first-token variance. Those are hypotheses. This log does not show which one.

### Why it matters to this project

M3 still does not explain T1–T2. A shorter reply can make the spoken turn feel faster without moving the first-text wait onto a stable fast path.

### Open questions

Per-turn transcription cost, server queue time, model identity, and why one warm turn took 44.8 s.

### Related milestone / decision

M2 remains PASSED. M3 not started. No architecture decision changed.

## 2026-10-13 — Latency investigation closeout

Evidence level: MEASURED, plus Muse-side transcription costs supplied as operator knowledge (0 s when not needed, about 2 s when a fallback runs). The device log still does not label transcription per turn.

### Question

Where does the remaining first-text wait sit, and is more client latency work justified before M3?

### What I learned

M1 added T0–T7 on the existing chat turn. T7 in firmware is `T7_PROXY`: the codec accepting the first non-silent write. It is not physical audible onset.

M2, before the Muse-side change, measured Chinese T1→T2 at a median of about 18.6 s (min 14.7 s, max 25.4 s) and release-to-playback-proxy at a median of about 19.5 s. English T1→T2 was about 12.9 s, and that reply was shown rather than spoken.

A Muse VM change removed unnecessary re-transcription. Transcription is about 0 s when it is not needed and about 2 s when the fallback runs. After that change, warm Chinese first-text times were no longer stuck near 19 s. A later warm set ran from 2.4 s to 14.5 s with one 44.8 s outlier (median 6.3 s). That outlier’s chat acknowledgement arrived about 0.4 s after T1. The client then waited about 44 s for the first text. No reconnect, retry, or local stall explained it.

A typed-versus-voice test used the same short question. Warm typed turns, after the server acknowledgement, completed a one-character reply in a band of about 3.2–5.0 s (median 3.56 s). Warm voice ack→first-text had median 5.69 s. About half of the voice turns sat in the same ~3 s band as typed text. The other half ran about 7–15 s. One accepted voice note produced no reply before the 60 s client timeout.

After T1, local work is the 2 ms poll and one JSON line: tens of milliseconds. Subscribe is already open before the user speaks. The voice-note body sends an empty `message`, `output_modality: "text"`, and the WAV. There is no model, reasoning, tool, or fast-mode field. Typed and voice share a ~3 s floor after accept. The voice long tail is larger than the known 0–2 s transcription cost, and the client cannot see the server stage that fills it.

The Muse-side VM change materially shortened the old ~19 s median. The remaining serving tail is outside current client visibility. Further client latency optimization is not justified until the server path changes.

### Why it matters to this project

M3 can evaluate speech quality, bilingual speech, and cancelability. It should not be treated as the fix for the multi-second first-text tail. Xiaole is still the temporary speaker. It is not the dominant latency.

### Open questions

Which server stage fills the post-accept floor and the voice-only tail. No client field currently selects a simpler turn.

### Related milestone / decision

M1 PASSED. M2 PASSED. Latency follow-up COMPLETE. M3 is ready to start and has not started. No architecture decision changed.
