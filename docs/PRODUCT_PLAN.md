# Product plan

Where we are going. This is the canonical product plan.

Live status is `PROJECT_STATUS.md`. How work is done is `ENGINEERING_PLAYBOOK.md`. What the firmware does today is `CURRENT_ARCHITECTURE.md`. This file does not claim that future subsystems exist.

## Product vision

The finished robot should feel like the user's Muse as a companion on the desk, not a gadget bolted to a robot body.

### A. Expressive character

A high-resolution avatar, close in spirit to that Muse character. States: idle, listening, thinking, working, speaking, happy, error, sleep/off. Petting and other touch change the expression. Tilt, shake, and pick-up change the expression and, once motion exists, the body. Avatar and body express the same moment.

### B. Natural conversation

Wake word or a manual start. After activation, a continuous multi-turn session: the user does not wake or press for every turn. Low latency, natural Chinese and English, fast time-to-first-text, fast time-to-first-audio, eventual interruption, and an automatic return to listening while the session is active. Readable mixed Chinese and English on the display, wrapping and paging long replies, shown as early as the text exists, with the avatar still present.

### C. Spatial interaction

When a person speaks, estimate their direction and turn the head toward them. This capability is already demonstrated on a StackChan + Muse setup, so it is a confirmed product goal. Also react to proximity, movement, and orientation. Camera use is on demand: one bounded capture when Muse asks, not a continuous vision loop.

### D. Voice-controlled embodiment

Natural language moves the head and body: look left or right, look at me, look up or down, nod, shake the head, turn by an angle, face forward. Commands go through a safe motion layer. Muse tool output does not write raw servo positions.

### E. Electronic pet behavior

Head petting, shake, tilt, and pick-up each get a reaction. The same gesture can differ with conversation state, motion intensity, and orientation. Personality shows up in those responses.

### F. Performance / personality

Choreographed dances synchronized with music, plus other expressive scripted behaviors. Dance is a timed routine, not a new character state.

## Product experience targets

| Target | Concrete goal |
|---|---|
| Fast response | Time the path from end of speech to first audible audio. See the T0–T7 marks in [Validation](#validation-strategy). |
| Natural TTS | A voice that is pleasant in Chinese and English. The current on-device voice is not this. |
| Chinese + English | Both languages spoken and shown. English must not be caption-only. |
| Readable display | Mixed Chinese and English, sensible wrapping, paging or scrolling, text on screen as soon as it exists. |
| Expressive avatar | The states in the vision, readable at a glance, coordinated with speech and with pet reactions. |
| Continuous session | After wake or a manual start, listening returns automatically until timeout, stop, sleep, or power policy. One press per turn is not the finished interaction. |
| Hands-free wake | A wake phrase, ideally "Hi Muse", can start that session. Manual start may ship first. Touch hold-to-talk can remain as a fallback. |
| Speaker orientation | Speech direction becomes a safe yaw target, and the head turns toward the active speaker. |
| Voice-controlled motion | Semantic gestures and angle commands share one safe motion API. |
| Electronic pet | Pet, shake, tilt, and pick-up produce expression and, later, body motion. Reactions can depend on state and intensity. |
| Performance | Music and motion follow one choreography timeline. Servos do not dance from audio amplitude. |
| Embodied behavior | Servo, IMU, proximity, and camera reactions tied to the same interaction state, after conversation quality is acceptable. |

## Current baseline

Board: M5Stack CoreS3 / StackChan, ESP32-S3, 16 MB flash. Toolchain: ESP-IDF v6.0.1 at `C:\esp\esp-idf-v6`.

This classification is the product bar. Whether a milestone is in progress is `PROJECT_STATUS.md`.

### Working and acceptable

- Boot, BLE pairing, and Wi-Fi. Switching Muse account is a phone re-pair, not a firmware change.
- Display comes up. The custom plush avatar is on the device and animates.
- Touch hold-to-talk, and a short tap still pets. A one-pixel slide no longer cancels the hold.
- App-only flashing leaves NVS in place, so pairing and Wi-Fi survive an app update.
- Factory recovery image: `archive/factory/stackchan-factory-backup-16MB.bin`, written at `0x0`. Flash offsets are in `DEVELOPMENT_HISTORY.md`. The partition table is at `0x10000`, not the ESP-IDF default `0x8000`.

### Working but prototype quality

- **On-device Chinese speech (esp-tts / xiaole).** It speaks, and it is wired into the Muse chat turn. It is not a finished feature. The voice is mechanical, English is not spoken on this path, and a reply can take more than 10 seconds. A later design may replace it. Detail: `VOICE_RESEARCH.md`.
- **Avatar.** The plush renderer works. It is not yet the high-resolution, state-synced face in the vision.
- **Captions.** Text can appear, including during the speech prototype. Readable Chinese conversation layout is not finished. A CJK font file exists and is not in the build. Detail: `DISPLAY_AND_CJK.md`.

### Not implemented yet

- Hands-free wake. Research: `WAKE_WORD_RESEARCH.md`.
- Continuous multi-turn conversation. Wake and the session loop are separate. Today's talk path is still one press per turn.
- High-quality Chinese and English TTS, early text, and interruption.
- Electronic-pet reactions beyond the existing short-tap happy face: shake, tilt, pick-up, and body motion with those reactions.
- Servo motion, voice-controlled gestures, IMU reactions, proximity/light behavior, camera as a Muse tool, SD, and LED behavior tied to conversation. Research: `BODY_HARDWARE_RESEARCH.md`.
- Head turn toward the active speaker. The product capability is confirmed by a StackChan + Muse demo. The port is not started, and the reference source is not yet identified in this tree.
- Choreographed, music-synchronized dance.

What the code does today: `CURRENT_ARCHITECTURE.md`.

## Target system direction

These pieces are not in the firmware today. The principle is: state semantics early; shared state infrastructure late. Phases 1, 3, 4, and 5 may locally know that a turn is listening, thinking, working, or speaking. They do not create `robot_behavior`, a behavior manager, a central event bus, or a `character_state` framework to do that. Add a shared subsystem only when more than one implemented output needs it.

Future inputs: touch, IMU, microphones, sound direction, proximity, camera, Muse tool or action requests, and continuous-conversation session state.

Future outputs: avatar, text, TTS and other audio, servo motion, LEDs, and music playback.

```
Muse interaction / tool state
            |
            v
      character / interaction state
       /      |      |      |      \
   avatar    text    TTS   motion   LEDs
                        \
                         music / performance
```

Four later subsystems are likely:

1. A shared character / interaction state. Phase 6 is when avatar, text, TTS, motion, and LEDs consume one state. Phase 6 does not invent the meanings. `working` may appear locally as soon as an action is executing.
2. A safe motion subsystem. Voice commands and speaker direction both end as a yaw or pitch target. Neither writes a raw servo position.
3. A conversation-session controller. Manual start and wake word only make the session active. The controller owns continuity. It does not own wake-word detection, speech recognition, Muse reasoning, TTS synthesis, tool logic, or servos.
4. A perception-input layer for touch, IMU, direction, proximity, and on-demand camera capture.

Character state and behavior are orthogonal. The state list is the interaction phase. An activity is what the robot is doing: orient toward the speaker, nod, shake the head, pet reaction, look left or right, return to neutral, dance, or a scripted gesture. Those activities are not extra character states. Valid pairs include happy while dancing, listening while orienting, speaking while nodding, and working while capturing a frame. A behavior layer may be useful later. It is not implemented, and it is not created until implemented features need it.

Music and performance stay a Phase 7 timeline. Dance is an activity, not a character state.

### Architecture implications

**A future Conversation Session Controller owns continuity.** Manual start and wake word only activate it. While the session is active, one bounded utterance becomes one Muse turn, then thinking, working, or speaking, then an automatic return to listening for a follow-up. Timeout, explicit stop, sleep, or power policy makes the session inactive. The controller also owns the follow-up window, interruption policy, and barge-in coordination. It does not own wake detection, recognition, reasoning, TTS synthesis, individual tools, or servos.

Session state and character state are orthogonal. An active session can be listening, thinking, working, or speaking. `working` does not by itself block the session. The target is that the session can stay receptive during working. No second enum or framework is added for this.

The first continuous-conversation implementation may keep using bounded voice-note turns. The current Hatch client already streams PCM chunks while the user holds, then sends the final chunk after release. A session can play the reply and listen again on that same turn shape. That is valid continuous conversation. Permanent realtime audio to the Muse cloud is not required. Current implementation fact: local capture stops at release or 15 seconds (`MAX_SECS` in `muse_voice.c`). An 8-second constant in the session (`IN_BYTES`) is mic-backlog capacity, not the utterance length. The client will accept 20 seconds of PCM and the recorder never reaches that. A comment says the server stops at 15 seconds; that server limit is unknown. Neither 8 nor 15 seconds is a future product requirement. Source notes: `VOICE_RESEARCH.md`.

**Speaking barge-in is a required target.** If the answer is wrong, irrelevant, or too long, the user speaks, local detection treats that as real user speech, current TTS stops, the new utterance is recorded, and a new Muse turn starts. This is local interruption awareness during playback, not a requirement for a permanent cloud audio stream. The current conversation loop writes the speaker and does not read the microphone while playback runs. There is no AEC or VAD on the product path. A press during playback only partly cancels audio: the software queue drains, and DMA may still play a short tail. Voice barge-in still needs capture during playback, a way to tell the user from the speaker, and a stronger playback cancel. A reference AEC/VAD exists in `references/xiaozhi-esp32` and is not integrated. Details: `VOICE_RESEARCH.md`.

**Working should ideally stay interruptible.** During "Write an email to John and send it," the user should be able to say stop, wait, don't send it, change that, or do this instead, and the current action can be modified or cancelled. An early session implementation may still suspend listening while working if concurrent listening is not yet safe. Target UX and that early limit are both accepted.

**Hearing an interruption and cancelling an action are different.** The session controller may emit a cancel request. Each action owns whether it can honor it. TTS playback is usually cancelable. Servo motion is usually stoppable through the safe motion API. `camera.capture` may cancel or its result may be ignored. A network or tool request depends on that tool. Sending email or another irreversible action may become non-cancelable after commit. Future actions need a way to be interruptible, cancelable, committed, or completed. That is not a framework, and no module is created for it now.

**Sound direction does not drive servos.** Direction estimate, then a filtered target, then the safe motion API, then the servo. Still to find by reading a reference, not by guessing: which implementation produces the estimate, whether it is ESP-SR AFE / DOA or something else, microphone geometry, where the direction is exposed, update rate, confidence, smoothing, how often the head may move, how direction maps to StackChan yaw, jitter, low-confidence behavior, and how this interacts with the session, wake word, and playback. Head motion may need to freeze in some states. That list is porting work. The capability itself is confirmed.

**Voice control uses the same motion API.** "Look left" and "speaker at +30 degrees" both become a safe yaw target. The layer enforces angle limits, speed limits, interpolation, smoothness where needed, a home pose, mechanical limits, stop / cancel, and no abrupt large reversal.

**Pet behavior and conversation must not fight.** If the robot is speaking and the user tilts it, a later design chooses whether that reaction interrupts, overlays, waits, or changes expression only. The rule is not fixed now.

**Dance uses one clock.** Music playback, servo targets, avatar expression, and optional LEDs share a choreography timeline. Servos do not move from audio amplitude. The file format is not chosen yet.

## Reference strategy

| Reference | Use |
|---|---|
| Muse upstream (`facebookincubator/muse-gadget-sdk`) | Protocol, chat, display, board bring-up. Remote `upstream`. |
| `references/Muse-charm-mosaico` | Voice-companion ideas: TTS submit/cancel, UI state, speech sync. Not a drop-in. |
| `references/StackChan-BSP` | First source for StackChan body hardware. |
| `references/StackChan` | Official StackChan application and motion examples. |
| `references/M5Unified` | CoreS3 pin tables when a new bus is needed. The current display and audio path stays on the Espressif CoreS3 BSP that Muse already uses. |
| `references/xiaozhi-esp32` | Secondary only: WakeNet, factory wake partition, and speech prior art. |

Which source wins, and how to attribute a copy, is in the workspace `WORKSPACE_STRUCTURE.md`.

## Product roadmap

Phases follow product dependency. Conversation quality comes before the body. Do not start servo bring-up, speaker-direction porting, dance, or IMU reactions until Conversation UX Foundation is acceptable.

### Phase 0 — Stable baseline

**Goal.** Keep the working Muse/CoreS3 system reproducible.

**Preserve.** Boot, pairing, Wi-Fi, display, custom avatar, touch hold-to-talk, the current speech prototype, and app-only flashing that does not wipe NVS.

**Status.** Mostly complete.

**Pass.** Existing behavior still reproduces after later changes.

### Phase 1 — Conversation UX

Highest priority. Do not assume xiaole stays. This phase is one turn done well: latency, natural TTS, time-to-first-text, time-to-first-audio, and text/audio sync. Sprint milestones are below. Barge-in belongs with the session loop in Phase 3.

**References.** This tree, `references/Muse-charm-mosaico`, Muse upstream, and `VOICE_RESEARCH.md`.

### Phase 2 — Display, avatar, and electronic pet reactions

**Display.** Readable Chinese and English, CJK font fallback, CJK line breaking, a usable size, paging or scrolling, the avatar still on screen.

**Avatar.** Higher-resolution reactions across the character states. The shared state layer is still Phase 6.

**Electronic pet, on the face first.** Extend today's short-tap happy reaction into a real pet. Design the avatar response for shake, tilt, and pick-up, including how intensity and orientation change it. Optional body motion waits until Phase 4 has a safe move. IMU capture itself is Phase 5. Notes: `DISPLAY_AND_CJK.md`.

### Phase 3 — Wake and continuous conversation

Wake method and session behavior are separate. This is a major feature. Phase 1 stays one turn done well. The session loop, automatic re-listen, wake word, and barge-in belong here.

**Session first.** Manual start activates a future Conversation Session Controller. That controller owns active versus inactive, the return to listening, the follow-up window, inactivity timeout, explicit stop, sleep or end, interruption policy, and barge-in coordination. A first implementation may be bounded, streamed voice notes. The client already uploads chunks while recording and a final chunk after release. The session then plays the reply and listens again. That is continuous conversation. It does not require a permanent cloud microphone stream. The firmware today stops a hold at about 15 seconds. That is a current implementation fact, not the product's duration requirement.

While speaking, barge-in is required target UX: the user talks, local detection accepts it as user speech, TTS stops, and the new utterance becomes the next turn. That still requires local capture during playback and a way to discriminate user speech. AEC and playback-aware detection are not in the product today. A button press during playback is only a partial cancel. While working, the target is that the user can still say stop or change the task. An early build may suspend listening during working if concurrent listening is not yet safe. The controller requests cancellation. Each tool decides whether the action is still interruptible or already committed.

**Wake second.** Voice wake, starting from a proven WakeNet model if needed, later aiming at "Hi Muse", activates that same session. It does not own the loop. Budget always-on mic, memory, and power. Keep pairing and Wi-Fi.

**Use.** ESP-SR / WakeNet, xiaozhi and StackChan prior art, and `WAKE_WORD_RESEARCH.md`. The old "append the model at `0x822000`" example is stale: that address is the xiaole `voice_data` partition.

### Phase 4 — Embodied motion and voice control

**Bring-up.** Yaw and pitch, a neutral home pose, limits, smooth interpolation. First test: neutral, a small move inside the published range, back to neutral.

**Safe motion API.** Every later move uses it. Tool output and model text never write a raw servo position.

**Voice control.** Natural-language intent becomes a Muse tool action, then a safe motion command. Semantic gestures and parameterized angles share that API. Speaker-direction turns in Phase 5 use it too.

Do not add a `robot_behavior` layer for this phase. Notes: `BODY_HARDWARE_RESEARCH.md`.

### Phase 5 — Spatial and sensor perception

**Speaker orientation.** Confirmed product capability: a StackChan + Muse demo already turns toward the person who is speaking. Remaining work is a port, not a feasibility study. Microphone input becomes a direction estimate, then a safe yaw target, then a smooth head turn, while listening continues. Do not invent the estimator, geometry, confidence metric, or yaw mapping until the reference source is identified and read.

**Camera.** On-demand `camera.capture` or another bounded inspection only. Muse decides a picture is needed, one image or a bounded capture is taken, the tool or model uses it, and the conversation continues. That execution is `working`. Continuous camera streaming, always-on scene understanding, always-running detection, and vision-driven autonomous behavior are out of scope. Reconsidering them requires a new product decision.

**Other sensing.** IMU, proximity and light, and sensor-driven reactions. Shake, tilt, and pick-up expressions designed in Phase 2 connect to real IMU events here, and to body motion only through the Phase 4 API.

### Phase 6 — Integrated character

Only after voice, display, the session loop, motion, and perception have each been validated on their own. This phase does not invent the state meanings. It is where those meanings become one shared state that avatar, text, TTS, motion, and LEDs all read. Do not add `robot_behavior`, a behavior manager, a central event bus, or a `character_state` framework before then.

**Character states.** idle, listening, thinking, working, speaking, happy, error, sleep/off. Activities such as dance, nod, and orient-to-speaker are not in this list.

- **thinking** — Muse is interpreting the request, reasoning, planning, forming an answer, or deciding which action or tool to use.
- **working** — Muse has already decided, and a task is executing. This is a general agent execution state, not a motion state. It includes a phone call, composing or editing, sending a message or email, an external tool, a network request, a file operation, a research workflow, `camera.capture`, a sensor read, robot motion, a longer tool, or any later agent workflow that has left reasoning and entered execution.
- **speaking** — Muse is presenting the result to the user.

During an active session, a plain question such as "Why is the sky blue?" is listening, thinking, speaking, listening. A task such as "Call John" or "Write me a short email and send it" inserts working after thinking. The session ends to idle or sleep/off on timeout, stop, sleep, power policy, or the user leaving.

```
listening -> thinking -> speaking -> listening
```

```
listening -> thinking -> working -> speaking -> listening
```

Avatar, text, TTS, servo motion, and LEDs read that same shared state. Touch, IMU, direction, proximity, an on-demand capture, and the session can feed it. Pet reactions and speaker turns become state-aware here. Earlier phases may already treat an executing task as `working` locally. Phase 6 is what makes that visible to every output.

For `working` specifically:

- **Avatar.** An active or busy animation, distinct from thinking.
- **Text.** A short status or progress line when that helps.
- **Motion.** Calm purposeful movement, not the speaking motion.
- **TTS.** Normally silent, unless an intermediate status is intentionally announced.
- **LEDs.** An active or busy indication, distinct from thinking and speaking.

### Phase 7 — Choreography and performance

After reliable audio playback, safe smooth servos, and Phase 6 coordination. Kept separate from Phase 6: a dance needs its own timeline shared by music, yaw and pitch targets, avatar expression, and optional LEDs. That clock is not the character-state layer.

Events on that timeline may later include a timestamp, yaw target, pitch target, avatar expression, an LED action, or a sound cue. The file format is not chosen yet. Motion still goes through the safe motion API. Servos do not follow audio amplitude.

## Validation strategy

Every milestone records: goal, user-visible behavior, reference sources, expected files, implementation steps, test procedure, pass criteria, regression checklist, and a Git checkpoint after it passes. The lifecycle is `ENGINEERING_PLAYBOOK.md`.

Latency work uses these marks. "Faster" is not a result.

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

Report each mark in milliseconds from T0, and the gap that dominates.

**Regression checklist for firmware changes in the current sprint**

- Boot reaches the avatar.
- The existing pairing and Wi-Fi still connect. Prefer an app-only flash.
- Hold-to-talk still starts a Muse turn. A short tap still pets.
- The custom avatar still draws.
- The xiaole prototype still speaks Chinese until a replacement is intentionally swapped in.
- `sdkconfig.token` and `muse_font_cjk_20.c` stay untracked.

## Current sprint

**Conversation UX Foundation.** Live status is `PROJECT_STATUS.md`. M1 has not started.

| ID | Milestone | Depends on |
|---|---|---|
| M1 | Latency instrumentation | Phase 0 baseline |
| M2 | Baseline measurements on the current path | M1 |
| M3 | TTS architecture evaluation | M1, plus `VOICE_RESEARCH.md`. Can overlap M2. |
| M4 | Chinese/English display foundation | Independent of the TTS choice. Can run beside M1–M3. |
| M5 | High-quality TTS prototype | M3. Do not start by extending xiaole unless M3 selects it. |
| M6 | Shorter time-to-first-audio | M5, measured with M1 |
| M7 | Text and voice state together | M4 and M5 |

M4 is in this sprint because readable text is part of conversation UX. It does not wait on the TTS decision. The generated font is on disk and is not in the build; flash size still has to be rechecked (`DISPLAY_AND_CJK.md`).

### M1 — latency instrumentation

- **Goal.** Log T0–T7 on one chat turn.
- **User-visible behavior.** None, beyond unchanged talk and speech.
- **References.** `muse_chat_session.cpp`, `muse_speak.c`, `VOICE_RESEARCH.md`.
- **Files.** Existing chat and speak files. No new subsystem.
- **Steps.** Timestamp the existing hooks. Print the eight marks on the serial log.
- **Test.** One Chinese hold-to-talk turn. Confirm all eight marks appear in order.
- **Pass.** A single turn produces T0–T7 with millisecond deltas.
- **Regression.** Checklist above.
- **Git.** Checkpoint only when asked, after the milestone passes.

### M2 — baseline measurements

- **Goal.** Record the current xiaole path, including turns over 10 seconds.
- **User-visible behavior.** Same as today.
- **References.** M1 log, `VOICE_RESEARCH.md`.
- **Files.** None beyond the M1 logs.
- **Steps.** Several Chinese turns and at least one English turn. Note which gap dominates.
- **Test.** Save the serial timestamps. English must show whether T4–T7 are skipped because this path does not speak English.
- **Pass.** A written baseline with per-gap times, not a single latency number.
- **Regression.** Checklist above.

### M3 — TTS architecture evaluation

- **Goal.** Choose the next speech architecture, or reject the current one with reasons.
- **User-visible behavior.** None until M5.
- **References.** `VOICE_RESEARCH.md`, Muse-charm-mosaico, upstream Muse, Espressif esp-tts.
- **Files.** A decision in `DECISIONS.md` when accepted. No firmware until the choice is made.
- **Pass.** One selected direction, with the rejected options named and the T0–T7 gaps it is meant to cut.

### M4 — Chinese/English display foundation

- **Goal.** A reply with mixed Chinese and English is readable, and the avatar remains visible.
- **User-visible behavior.** Larger text, correct wrapping, a way to see a long reply.
- **References.** `DISPLAY_AND_CJK.md`, upstream CJK research, the current caption path.
- **Files.** Font asset plus the UI file that draws captions. The generated file is 4,463,636 bytes of C source. The research estimates about 693 KB of uncompressed font data. Confirm the linked size before wiring it in.
- **Pass.** Text is readable, Chinese breaks on character boundaries, and the avatar stays on screen.
- **Regression.** Checklist above. Avatar pixels unchanged when the reply is short.

### M5 — high-quality TTS prototype

- **Goal.** One new speech path that speaks Chinese and English.
- **User-visible behavior.** A reply is heard in the language Muse used, and it does not sound like the xiaole prototype.
- **References.** The M3 decision, `VOICE_RESEARCH.md`.
- **Files.** Whatever M3 names. Leave a way to restore the xiaole path until the new path is proven.
- **Pass.** Both languages produce audible speech. Quality is acceptable for a prototype of the product voice, which xiaole is not.
- **Regression.** Pairing, hold-to-talk, avatar. If the new path fails, the device still shows text.

### M6 — shorter time-to-first-audio

- **Goal.** Cut the dominant T0–T7 gap found in M2, on the M5 path.
- **User-visible behavior.** Speech starts earlier on a comparable sentence.
- **Pass.** A before/after table of T0–T7. The chosen gap is smaller, and the other gaps are still measured.

### M7 — text and voice state together

- **Goal.** Text appears as early as T3 allows, and the avatar shows listening, thinking, and speaking with the turn.
- **User-visible behavior.** Captions do not wait for full playback. The face matches the phase of the turn.
- **Pass.** On one conversational turn, text is visible before T7, and the avatar state changes for listening, thinking, and speaking. A shared `working` presentation across outputs is Phase 6. It is not part of this sprint's pass bar.
