# Project status

Where we are now. Update this file when a milestone starts, blocks, or passes. The product direction stays in `PRODUCT_PLAN.md`.

## Current sprint

Conversation UX Foundation

## Current milestone

**READY TO START**

M3 — TTS architecture evaluation

M1 — PASSED. T0–T7 are on the existing chat turn. T7 in firmware is a playback-write proxy, not physical audible onset.

M2 — PASSED. The pre-change Chinese T1→T2 median was about 18.6 s. After the Muse VM transcription change, first text is often a few seconds and still has a long tail.

Latency follow-up — COMPLETE. Client work after the note is closed is tens of milliseconds. The remaining multi-second wait is server-side from the client’s perspective. Further client latency optimization is paused. The pause is because that wait is outside current client control, not because the latency target is already met.

M3 implementation has not started. Xiaole is still the temporary speaker.

## Overall status

READY FOR M3

M3 has not started.

## Last known-good product commit

`8b023fd3575f82e875d895725f396e9cf6aea147` on `main` (`docs: organize product roadmap and engineering research`).

That commit does not change firmware. The firmware baseline inside it is `93a2b7a0adad7eb96df5cca5b9aef56a9f1a4b5a`.

## Confirmed baseline

These behaviors are the known-good checklist. "Working" here means the behavior has been brought up. It does not mean the product experience is finished.

| Behavior | Working | Product-quality complete |
|---|---|---|
| Boot | yes | yes, for this baseline |
| Muse pairing | yes | yes, for this baseline |
| Wi-Fi | yes | yes, for this baseline |
| Display | yes | yes, for bringing up the UI |
| Custom avatar | yes | no — not the final expressive face |
| Touch hold-to-talk | yes | yes, as the current talk control |
| Local xiaole speech | yes | no — prototype only |

## Current product gaps

- Natural high-quality TTS
- Conversation latency
- Chinese and English conversation display
- Continuous conversation
- Wake word
- Electronic-pet sensor reactions
- Safe servo motion
- Voice-controlled motion
- Sound-source orientation port
- Perception
- Unified character coordination
- Choreography

## Active architecture questions

- What architecture should replace the current xiaole TTS prototype?
- Which server stage fills the post-accept first-text floor and the voice-note long tail? Client polling, the JSON parser, and xiaole are not that wait. Further client latency work is deferred.
- What should the safe motion API look like?
- Which demonstrated StackChan + Muse implementation should be the sound-source-localization reference?

The voice-note length question is closed. Local capture stops at release or 15 seconds. The 8-second constant is mic backlog, not the utterance limit. Notes: `VOICE_RESEARCH.md`.

## Phase 3 research (not blockers for M1)

- Whether CoreS3 can reliably capture the microphone while speaker playback runs. The conversation loop does not do this today. Board init alone does not prove it.
- Which AEC or playback-aware VAD integration to use. None is in the product. ESP-SR AFE in `references/xiaozhi-esp32` is reference-only.
- How fully playback can be cancelled, including a codec or DMA flush. Today's press-to-cancel only drains the software queue.
- The actual server-side maximum note duration, if it ever matters. A source comment says 15 seconds. That is not a measurement.

## Pending architecture feedback

None open. Accepted review items are recorded below and in `DECISIONS.md`. They are not implementation work.

### Feedback AF-001

Feedback: State semantics early; shared state infrastructure late.

Why it matters: Phases can name listening, thinking, working, and speaking without a framework.

Affected system / phase: Phases 1, 3, 4, 5, and 6.

Existing assumption challenged: That using the vocabulary required a `character_state` subsystem.

Options: Introduce the shared object now, or name the phases locally and share them in Phase 6.

Decision status: ACCEPTED

Recorded as ADR-010.

### Feedback AF-002

Feedback: `working` is general agent execution, not robot motion, and Phase 6 does not invent it.

Why it matters: Calls, mail, tools, and motion are the same phase of a turn.

Affected system / phase: Character-state semantics. Shared visibility is Phase 6.

Existing assumption challenged: That `working` meant servos, or existed only after Phase 6.

Options: Motion-only working, or execution-wide working.

Decision status: ACCEPTED

Recorded as ADR-011.

### Feedback AF-003

Feedback: Camera scope is on-demand capture only.

Why it matters: Continuous vision is a different cost and privacy problem.

Affected system / phase: Phase 5.

Existing assumption challenged: That a camera goal implied an always-on perception loop.

Options: Bounded `camera.capture`, or continuous autonomous vision.

Decision status: ACCEPTED

Recorded as ADR-012. Continuous autonomous vision is out of scope.

### Feedback AF-004

Feedback: Character state and behavior are orthogonal.

Why it matters: Dance, nod, and orient-to-speaker must not become enum values.

Affected system / phase: Phases 4, 5, 6, and 7.

Existing assumption challenged: That every activity belongs in the character-state list.

Options: Grow the enum, or keep activity separate.

Decision status: ACCEPTED

Recorded as ADR-013. No behavior manager is created.

### Feedback AF-005

Feedback: A future Conversation Session Controller owns continuity. Wake word and manual start only activate the session.

Why it matters: The follow-up loop, timeout, and barge-in policy need one owner that is not the wake detector.

Affected system / phase: Phase 3.

Existing assumption challenged: That wake or the talk button owns the multi-turn loop.

Options: Put continuity in the wake path, or in a session controller that both start methods activate.

Decision status: ACCEPTED

Recorded as ADR-014. Bounded, streamed voice-note turns are a valid first implementation. The client already uploads chunks while recording. No controller exists.

### Feedback AF-006

Feedback: Session state and character state are orthogonal. `working` does not block the session by definition.

Why it matters: An active session can be listening, thinking, working, or speaking.

Affected system / phase: Phase 3 and character-state semantics.

Existing assumption challenged: That execution and an open conversation are the same flag.

Options: One combined state, or two dimensions with no new framework yet.

Decision status: ACCEPTED

Recorded as ADR-015. Semantic only.

### Feedback AF-007

Feedback: The user must eventually be able to interrupt the robot while it is speaking.

Why it matters: A wrong or long answer should be correctable immediately.

Affected system / phase: Phase 3. Not Phase 1.

Existing assumption challenged: That barge-in required a permanent realtime cloud audio stream.

Options: Full-duplex cloud audio, or local interruption of playback around bounded turns.

Decision status: ACCEPTED

Recorded as ADR-016. AEC and playback-aware detection are not in the product. A press during playback is only a partial cancel. Voice barge-in remains Phase 3 research.

### Feedback AF-008

Feedback: Working should ideally stay interruptible. An early session may still suspend listening during working.

Why it matters: "Wait — change tomorrow to Friday" is the target, without blocking the first session loop on concurrent listening.

Affected system / phase: Phase 3, and later tool cancellation.

Existing assumption challenged: That interruptible working had to ship with the first continuous session.

Options: Require concurrent listening immediately, or keep it as target UX with an early exception.

Decision status: ACCEPTED

Recorded as ADR-017. Hearing an interruption and cancelling the action remain separate. No action framework is created.

## Next action

Start M3 — TTS architecture evaluation when asked. Implementation has not started. Latency optimization stays paused.

## Blockers

None.

## Regression baseline

- Boot
- Pairing
- Wi-Fi
- Display
- Avatar
- Hold-to-talk
- Existing speech prototype

## Last updated

2026-10-13. M1 PASSED. M2 PASSED. Latency follow-up COMPLETE. Current milestone is M3, status READY TO START. M3 implementation has not started.
