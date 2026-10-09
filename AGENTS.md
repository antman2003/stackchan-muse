# Product identity

This is the StackChan/CoreS3 product built on the Meta Muse Gadget SDK.

Target: M5Stack CoreS3 / StackChan.

Product branch: `main`.

Current baseline: `93a2b7a`.

Build and flash details stay in `esp32/AGENTS.md`. Board-porting details stay in `esp32/devices/AGENTS.md`.

# Architecture

The Muse SDK remains the application and protocol architecture.

Prefer this direction:

Muse application/events → robot/interaction layer → hardware abstraction → StackChan/CoreS3 hardware.

Avoid scattering direct hardware access through shared Muse code.

# Working behavior to preserve

- boot
- Muse pairing
- Wi-Fi
- display
- custom avatar
- animation
- local speech using sibling `../esp-tts`

# Product-specific source guidance

Current product changes primarily live around:

- `esp32/components/muse/`
- board and device configuration
- `esp32/partitions_muse.csv`

Prefer a product-specific file or overlay over modifying broad shared Muse code when practical.

# Hardware-source clarification

For CoreS3 hardware already used by the Muse board implementation, use the BSP or source already referenced by the Muse board file first.

For StackChan body hardware, use:

1. `references/StackChan-BSP`
2. `references/StackChan`
3. `references/M5Unified`

Do not reclone vendor repos that already exist under `references/`.

# Upstream relationship

`upstream/main` is the official Meta Muse SDK.

Do not alter upstream agent documentation just to carry StackChan product policy.

Upstream update flow: fetch upstream, inspect changes, use a temporary update branch, build and test, then merge into `main` only after validation.
