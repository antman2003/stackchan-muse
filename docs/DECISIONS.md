# Architecture and product decisions

Durable choices only. Unresolved implementation details stay as questions in `PROJECT_STATUS.md`.

## ADR-001 — Muse Gadget SDK is the application backbone

Status: ACCEPTED

Date: 2026-10-10

Context: The robot needs Muse pairing, chat, display, and the CoreS3 board path that already runs.

Decision: `stackchan-muse` remains a product branch of the Muse Gadget SDK. New behavior extends that tree. It does not replace it with another firmware.

Why: Pairing, Wi-Fi, the display, and the talk path already work on this base.

Alternatives considered: A separate StackChan firmware with Muse added beside it. Rejected for this product because the working system is already the SDK.

Consequences: Upstream files stay shared. Product edits in those files stay small. `upstream` is Meta's repository and is never pushed.

Revisit when: Muse chat can no longer host the session, speech, or tool behavior the product needs.

## ADR-002 — Keep the upstream CoreS3 platform path

Status: ACCEPTED

Date: 2026-10-10

Context: Display and audio already use the Espressif CoreS3 BSP that the Muse board file includes.

Decision: Preserve that platform path. Use M5Unified pin tables when a new bus is needed. Do not re-home the current display and audio path onto another BSP.

Why: The known-good baseline depends on that path. Replacing it is not required for the product goals.

Alternatives considered: Driving the existing panel and codec through M5Unified. Not accepted.

Consequences: Body hardware is added beside this path, starting from StackChan-BSP, then StackChan.

Revisit when: A required bus cannot be brought up without leaving the current BSP.

## ADR-003 — Xiaole is a prototype, not the product voice

Status: ACCEPTED

Date: 2026-10-10

Context: On-device esp-tts / xiaole speaks Chinese and is wired into the chat turn.

Decision: Do not mark spoken replies complete. Do not assume xiaole remains the speech engine. A later milestone may replace it.

Why: The voice is mechanical, English is not spoken on this path, and end-to-end replies can take more than 10 seconds.

Alternatives considered: Treating the current path as the finished TTS feature. Rejected.

Consequences: Phase 1 measures the path, then chooses a TTS architecture. The prototype stays until a replacement is proven.

Revisit when: M3 selects a direction.

## ADR-004 — No central behavior layer yet

Status: ACCEPTED

Date: 2026-10-10

Context: Avatar, captions, and the speech prototype are hooked from existing Muse files. Servos and sensors are not driven.

Decision: Do not add `robot_behavior` or a `character_state` implementation until more than one implemented system needs the same state.

Why: A layer with one client is indirection. Voice, display, wake, motion, and perception are validated on their own first.

Alternatives considered: Introducing the full character graph before Phase 1. Rejected.

Consequences: Product behavior stays next to the code that owns it. Phase 6 is the shared-infrastructure point, not the starting architecture. Local use of the state names is allowed earlier. See ADR-010.

Revisit when: Avatar, text, voice, and body are all implemented and need one shared state.

## ADR-005 — Wake and the conversation session are separate

Status: ACCEPTED

Date: 2026-10-10

Context: The finished interaction is a multi-turn session. Hands-free "Hi Muse" is still a goal. Today is one press per turn.

Decision: The wake method only starts a session. The session is the listening, thinking, speaking, listening loop, ended by timeout, stop, sleep, power policy, or the user leaving. A manual start may ship before the wake phrase.

Why: Continuous conversation is a major feature and does not depend on a custom wake word.

Alternatives considered: Blocking the session loop on "Hi Muse". Rejected.

Consequences: Phase 3 builds the session first, then wake. Barge-in belongs with that loop, not with the single-turn latency work in Phase 1. ADR-014 names the controller that owns the loop.

Revisit when: The session loop exists and a wake phrase is ready to open it.

## ADR-006 — Future character states

Status: ACCEPTED

Date: 2026-10-10

Context: The face already has modes. The product needs a shared vocabulary before several outputs follow one interaction.

Decision: The future set is idle, listening, thinking, working, speaking, happy, error, and sleep/off. Thinking means Muse is deciding. Working means a chosen task is executing, including software and agent tasks, not only motion. Speaking means the result is being presented. A plain turn is listening, thinking, speaking, listening. A tool turn inserts working. Dance is an activity, not a state in this set. ADR-011 states the execution meaning in full. ADR-013 keeps activities out of this set.

Why: Tool execution is a different wait from reasoning, and a dance timeline is not a conversation state.

Alternatives considered: Adding `dancing` as a core state. Not accepted.

Consequences: Phase 6 uses this set. Earlier phases must not pretend the shared object exists. This decision names the set. It does not add the type to the firmware.

Revisit when: Phase 6 starts, or a real behavior cannot be expressed with this set.

## ADR-007 — One safe motion API for voice and speaker direction

Status: ACCEPTED

Date: 2026-10-10

Context: "Look left" and "the speaker is at +30 degrees" both need to move the head. Speaker orientation is a confirmed demo capability whose port is not identified.

Decision: Both become a safe yaw or pitch target. They do not grow separate servo writers.

Why: Two writers will fight, and both need the same limits.

Alternatives considered: Letting the direction estimator command the servo bus itself. Rejected.

Consequences: Phase 4 creates the API. Phase 5's port consumes it. The estimator, geometry, and yaw mapping are not decided here.

Revisit when: The reference source for direction is identified, or the API cannot express a required gesture.

## ADR-008 — No raw servo positions from the model

Status: ACCEPTED

Date: 2026-10-10

Context: Natural language will eventually request motion. The body uses serial servos with mechanical limits. This firmware does not drive them yet.

Decision: LLM or tool output never writes a raw servo position. Every command passes through a safe motion layer: angle limits, speed limits, interpolation, smoothness where needed, a home pose, mechanical limits, stop / cancel, and no abrupt large reversal.

Why: A model angle can stall a servo or reverse the bus hard enough to hang it.

Alternatives considered: Mapping tool fields straight to a position write. Rejected.

Consequences: Phase 4's first motion is a small neutral-and-return test, then the API, then voice commands. The serial protocol details stay in `BODY_HARDWARE_RESEARCH.md` until that phase.

Revisit when: A measured servo behavior shows the limit set is wrong.

## ADR-009 — Dance follows a timeline, not audio amplitude

Status: ACCEPTED

Date: 2026-10-10

Context: The product includes choreographed dances synchronized with music.

Decision: Music playback, servo targets, avatar expression, and optional LEDs share one choreography timeline. Servos do not move from raw audio level. The file format is not chosen. Dance stays Phase 7, after playback, safe motion, and character coordination exist.

Why: Amplitude following looks random. A shared clock is what makes a routine intentional. The character-state layer is the wrong place to own that clock.

Alternatives considered: Folding dance into Phase 6, or adding a `dancing` state. Not accepted.

Consequences: Phase 7 is separate. Motion still uses the safe motion API.

Revisit when: A concrete routine cannot be expressed as timed targets on that shared clock.

## ADR-010 — State semantics early, shared state infrastructure late

Status: ACCEPTED

Date: 2026-10-10

Context: The product has a future interaction vocabulary, and several phases will need to know whether a turn is listening, thinking, working, or speaking. No shared character-state object exists in the firmware.

Decision: Define and use the state meanings early. Do not introduce a centralized shared character-state subsystem until multiple implemented outputs need one. Phase 6 is that point: avatar, text, TTS, motion, and LEDs. Earlier code may know the phase locally. It does not create `robot_behavior`, a behavior manager, a central event bus, or a `character_state` framework.

Why: Naming a phase is cheap. A framework with one client adds coupling before there is a second consumer.

Consequences: State semantics may show up in Phases 1, 3, 4, and 5. Shared infrastructure stays Phase 6. Nothing in this decision is implemented.

Revisit when: Avatar, text, TTS, motion, and LEDs are each implemented and disagree about the phase of a turn.

## ADR-011 — Working is a general agent execution state

Status: ACCEPTED

Date: 2026-10-10

Context: `working` could be misread as "the robot is moving" or as a state that appears only when Phase 6 builds a shared object.

Decision: Thinking means Muse is interpreting, reasoning, planning, forming an answer, or choosing an action. Working means the action is already chosen and a task is executing. Speaking means the result is being presented to the user. Working covers software and agent work as well as physical work: a call, composing or editing, a message or email, an external tool, a network request, a file operation, a research workflow, `camera.capture`, a sensor read, robot motion, or a later agent workflow. A knowledge question skips working. "Call John" and "Write me a short email and send it" include it. Phase 6 does not invent this meaning. It makes working visible to avatar, text, TTS, motion, and LEDs.

Why: The user is waiting on execution, not only on reasoning, for many tasks that never move a servo.

Consequences: Earlier phases may mark local execution as working. No execution framework is added by this decision.

Revisit when: A real Muse task cannot be described as reasoning, execution, or presentation.

## ADR-012 — Camera is on demand only

Status: ACCEPTED

Date: 2026-10-10

Context: Phase 5 includes a camera so Muse can look when asked. Continuous vision would be a different product.

Decision: Camera use is on demand. Muse decides an image is needed, `camera.capture` or another bounded capture runs, the tool or model uses that result, and the conversation continues. That can happen during working. Continuous streaming, always-on scene understanding, always-running detection, and vision-driven autonomous behavior are out of scope.

Why: A continuous vision loop costs compute, memory, power, bandwidth, and latency, and it raises privacy and architecture questions the current product does not need.

Consequences: Phase 5 camera work is a bounded capture tool. It is not implemented. Reopening continuous vision requires a new decision.

Revisit when: A product goal cannot be met with a bounded capture.

## ADR-013 — Character state and behavior are orthogonal

Status: ACCEPTED

Date: 2026-10-10

Context: Dance, nod, orient-to-speaker, and pet reactions are easy to mistake for extra character states.

Decision: Character state is the interaction phase: idle, listening, thinking, working, speaking, happy, error, sleep/off. Behavior is the activity: orient-to-speaker, nod, shake head, pet reaction, look left or right, return to neutral, dance, or a scripted gesture. Activities stay out of the state list. A state and an activity may combine, such as happy while dancing, listening while orienting, speaking while nodding, or working during `camera.capture`. Do not add a behavior manager, activity manager, `robot_behavior`, or central event bus until implemented features require one.

Why: Putting every activity in the state enum makes exclusive phases out of things that can overlap.

Consequences: This is a distinction only. No behavior layer is implemented.

Revisit when: Two implemented activities need a coordinator that the safe motion API and the local phase cannot provide.

## ADR-014 — A Conversation Session Controller owns continuity

Status: ACCEPTED

Date: 2026-10-10

Context: Continuous conversation is a loop of turns. Manual start and wake word are ways into that loop. Today is still one press per turn.

Decision: A future Conversation Session Controller owns continuity: active or inactive, automatic return to listening, the follow-up window, inactivity timeout, explicit stop, sleep or end, interruption policy, and barge-in coordination. Manual start and wake word only activate the session. The controller does not own wake-word detection, speech recognition, Muse reasoning, TTS synthesis, individual tool logic, or servo implementation. The first implementation may use bounded voice-note turns: one utterance, one Muse turn, play the reply, then listen again. The current client already streams that note while it is recorded and sends the last chunk after release. That is valid continuous conversation. Permanent realtime streaming to the Muse cloud, full-duplex cloud audio, and a continuously open cloud microphone are not required.

Why: Continuity is a session policy. Mixing it into the wake detector or the speech engine makes both harder to change.

Consequences: Phase 3 is where this controller is introduced. It is not implemented. The controller may request cancellation of the current presentation or action. Each tool owns whether that action is still interruptible, cancelable, already committed, or completed. No action framework is created for that.

Source note (2026-10-11, not a new decision): local capture stops at release or 15 seconds (`MAX_SECS` in `muse_voice.c`). `IN_BYTES` is 8 seconds of mic backlog and does not end the recording. Client capacity is 20 seconds. A comment says the server stops at 15 seconds; that server behavior is unknown. See `VOICE_RESEARCH.md`. These are implementation facts. They do not set a product duration.

Revisit when: Bounded or streamed turns cannot provide the session UX, or a measured server limit forces a different capture model.

## ADR-015 — Session state and character state are orthogonal

Status: ACCEPTED

Date: 2026-10-10

Context: An active conversation and the phase of a turn are easy to collapse into one flag.

Decision: Session state and character state are different dimensions. An active session may be listening, thinking, working, or speaking. `working` does not automatically mean the session is blocked. The product keeps the option for the session to stay receptive during working. No separate enum or framework is added for this.

Why: A task can be executing while the user is still allowed to speak. Blocking the session whenever the character is working would throw that option away.

Consequences: This is a semantic decision only. ADR-017 says an early implementation may still suspend listening during working if concurrent listening is not yet safe.

Revisit when: A platform limit makes a receptive session during working impossible.

## ADR-016 — Speaking barge-in is a required target UX

Status: ACCEPTED

Date: 2026-10-10

Context: A wrong, irrelevant, or long answer should not have to finish before the user can correct it.

Decision: While the robot is speaking, the user must eventually be able to interrupt. Target path: the user starts speaking, local detection accepts that as real user speech, current TTS presentation stops, the new utterance is recorded, and a new Muse turn is submitted. This does not require permanent realtime audio to the cloud. Bounded voice-note turns may remain. What is required is local interruption awareness during playback. AEC, playback-aware speech detection, microphone behavior during speaker output, simultaneous CoreS3 input and output, and fast cancel on the chosen TTS path are research questions. None of them are claimed to work today.

Why: Waiting out a bad answer breaks the conversation.

Consequences: Barge-in belongs to Phase 3, not Phase 1. The likely hard part is separating the user's voice from the robot's speaker. No VAD, AEC, or cancel path is added by this decision.

Source note (2026-10-11, not a new decision): the conversation loop does not read the microphone while it writes playback. No AEC or VAD was found on the CoreS3 product path. A press during playback calls `muse_hatch_turn_cancel` and drains the software queue. That cancel is partial: there is no dedicated speak-cancel and no codec or DMA flush. ESP-SR AFE with AEC and VAD exists only as a reference in `references/xiaozhi-esp32/main/audio/engines/afe_audio_engine.cc`. See `VOICE_RESEARCH.md`.

Revisit when: A measured audio path shows that user speech cannot be detected during playback on this hardware.

## ADR-017 — Working should ideally stay interruptible

Status: ACCEPTED

Date: 2026-10-10

Context: During a task such as "Write an email to John and send it," the user may say stop, wait, don't send it, change that, or do this instead.

Decision: The target UX is that `working` stays receptive, so the current action can be modified or cancelled. An early continuous-conversation implementation may suspend listening while working if the platform cannot yet listen safely at the same time. Detecting the interruption and cancelling the action stay separate: the session may request cancel, and the tool decides whether the action is still interruptible or already committed.

Why: The ideal companion accepts a correction mid-task. The first session loop should not be blocked on solving concurrent listening.

Consequences: Target UX and the early exception are both accepted. No cancellation module is created.

Revisit when: The session loop exists and concurrent listening during working is either measured safe or measured unsafe.
