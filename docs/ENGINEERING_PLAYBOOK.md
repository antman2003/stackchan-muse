# Engineering playbook

How we work. Product direction is `PRODUCT_PLAN.md`. Current status is `PROJECT_STATUS.md`. Learnings go in `LEARNING_LOG.md`. Durable choices go in `DECISIONS.md`. What the firmware does today is `CURRENT_ARCHITECTURE.md`. Research docs hold the evidence. Do not copy those into each other.

## Documentation ownership

Outside the product Git repo:

- Workspace `README.md` — short entry point and links only.
- `WORKSPACE_STRUCTURE.md` — folders, repositories, and source priority.

Inside `stackchan-muse/docs/`:

| File | Owns |
|---|---|
| `PRODUCT_PLAN.md` | Where we are going |
| `PROJECT_STATUS.md` | Where we are now |
| `ENGINEERING_PLAYBOOK.md` | How we work |
| `LEARNING_LOG.md` | What we learned |
| `DECISIONS.md` | Why this architecture |
| `CURRENT_ARCHITECTURE.md` | What the firmware does today |
| Research docs | Prior art and measured detail |

## Core principles

- Preserve the known-good baseline.
- Prefer small isolated changes.
- Read current product code before references.
- Read the appropriate local reference before implementing unfamiliar hardware.
- Never guess hardware configuration.
- Separate observation from interpretation.
- Measure performance before optimizing it.
- Avoid drive-by refactoring.
- Avoid premature abstraction.
- Minimize edits to shared upstream Muse files.
- One milestone should prove one main thing.

## Evidence taxonomy

Use these labels in status, learning, and decisions.

| Label | Meaning |
|---|---|
| MEASURED | Observed on this hardware, or taken from its logs. |
| SOURCE-CONFIRMED | Read in this tree or in a named reference file. |
| DEMO-CONFIRMED | Seen in a relevant working demo. Our port is not identified or verified yet. |
| HYPOTHESIS | Plausible, and not demonstrated in this project. |
| DECISION | An approved product or architecture direction. |
| PROTOTYPE | Works, and does not yet meet the product bar. |

Sound-source localization is DEMO-CONFIRMED as a capability. The implementation in this repo is still unresolved.

Current xiaole speech is PROTOTYPE.

## Standard milestone lifecycle

1. User-visible goal
2. Current baseline
3. Relevant references
4. Questions and hypotheses
5. Test or instrumentation plan
6. Expected files to change
7. Smallest implementation
8. Build
9. Flash and run
10. Test
11. Compare against pass criteria
12. Regression test
13. Record learning
14. Record an architecture decision if one was made
15. Update `PROJECT_STATUS.md`
16. Git checkpoint after the milestone passes

## Architecture feedback process

User feedback does not immediately change code.

```
User feedback
      |
      v
PROJECT_STATUS.md  Pending Architecture Feedback
      |
      v
investigation and options, if needed
      |
      v
user decision
      |
      v
DECISIONS.md, if the choice should last
      |
      v
PRODUCT_PLAN.md, if the direction changed
      |
      v
implementation milestone
```

Each pending item uses:

### Feedback XXX

Feedback:

Why it matters:

Affected system / phase:

Existing assumption challenged:

Options:

Decision status: PENDING / ACCEPTED / REJECTED / NEEDS INVESTIGATION

## Progress update rule

At the start of a milestone, set `PROJECT_STATUS.md` to that milestone, its goal, and status IN PROGRESS.

During investigation:

- learning goes to `LEARNING_LOG.md`
- architecture feedback goes to Pending Architecture Feedback
- detailed evidence goes to the relevant research doc

When a decision is accepted, add it to `DECISIONS.md`.

When implementation passes, set the milestone to PASSED, update the regression baseline, and record the last-known-good commit after the checkpoint. Then move to the next milestone.

Do not commit every small status edit. Checkpoint a tested milestone or a durable documentation decision.

## Refactor rule

Refactor only when at least one of these is true:

- a current feature requires it
- the same coupling already showed up more than once
- more than one implemented system needs the same abstraction
- an upstream conflict has become a real problem
- the behavior cannot be tested or kept reliable otherwise

Do not refactor because a diagram would look cleaner.

## Reference usage

Reference repositories stay read-only unless a task explicitly says otherwise.

When a reference is used, record:

- repository
- exact file
- concept
- copied, adapted, or conceptual
- license relevance

## Git checkpoints

Commit after a tested milestone passes, or after a durable documentation decision, and only when asked.

Do not commit `sdkconfig.token`, `muse_font_cjk_20.c`, factory images, or token-bearing firmware. Do not push to `upstream`.
