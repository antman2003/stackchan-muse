# Body hardware research

> Moved from the workspace lab notebook on 2026-10-09.
> Device identifiers (SSID, LAN address, MAC, BLE name, and a personal name in a sample reply) are replaced with placeholders.
> Cross-links that pointed at old README anchors are rewritten below where they would break.

## Status

Body hardware is **not implemented** in this firmware. Research only. Do not start servo or body work until conversation UX is acceptable.

Servo safety findings in this file are design constraints, not measured limits from this robot. **needs re-verification** on the bench before any motion: start at neutral, move a small amount inside the published range, and return to neutral.

### 7. Body hardware: servos, camera, sensors, SD [8] — research first, then discuss

The original StackChan firmware uses all of this, so none of it is in doubt
hardware-wise. **Muse firmware simply does not know the base exists** — all of
it is net-new code. Research findings below; agree an approach before building.

#### The base is a separate device on its own buses

The robot body (not the CoreS3) carries the servos, LEDs, IR, touch strip and
NFC, reached over pins the main unit exposes on its bottom connector:

| Function | Pin |
|---|---|
| Servo_TX | G6 |
| Servo_RX | G7 |
| IR_SEND / IR_REC | G5 / G10 |
| Base I2C SCL / SDA | G11 / G12 |

The base has its **own I2C bus** (G11/G12), separate from the CoreS3 internal
bus that carries AXP2101, AW9523B, FT6336U and ES7210:

| Base I2C device | Address |
|---|---|
| NFC ST25R3916 | 0x50 |
| Three-zone touch panel Si12T | 0x68 |
| IO expander PY32L020 | 0x6F (or 0x71) |

**Verify none of G5-G12 is claimed by the `espressif/m5stack_core_s3` BSP
v4.1.0 before wiring anything.** Muse's board file uses BSP macros
(`BSP_I2S_SCLK`) rather than raw GPIO numbers, so the pin map lives in that
managed component, which is only downloaded at first build.

#### Prior art: carbeso/stackchan-mcp solved this already

[carbeso/stackchan-mcp](https://github.com/carbeso/stackchan-mcp) is an MCP
gateway for **the exact M5Stack official StackChan kit** (2025 Kickstarter). It
runs xiaozhi firmware, not Muse, so it is not a drop-in — but its servo
findings are directly reusable and were paid for with on-device testing:

- **Servo power must be enabled.** It exposes a `check_vm_en` tool — "Check
  servo power supply (VM EN HIGH) state". Servos will not move until VM EN is
  asserted. Easy to lose hours to.
- **Two-tier pitch clamping, empirically measured:**

| Tier | Range | Why |
|---|---|---|
| Hard clamp | `0..88°` | 0° leaves ~1° above the validated mechanical end-stop; 88° sits ~1° inside the **audible sub-stall boundary found at 89°** — a "ji-ji-" gear-strain sound during an on-device sweep |
| Recommended | `5..85°` | M5Stack's documented range; outside it is not instantly damaging but stresses the servo over time |

- **Yaw is `-90..+90°`** with no hardware restriction, per M5Stack.
- **The servo bus can hang on large abrupt reversals** (e.g. +60° to -60°).
  Their fix is interpolation in a motion update task. So a motion task that
  ramps toward targets is required, not optional.
- Clamp in **both** places — at the API boundary and in the low-level handler.

**Licensing trap:** Feetech's `SCServo_lib` (`SCS.cc`, `SCSCL.cc`, `SCSerial.cc`)
is **GPL-3.0**, and linking it makes the whole firmware binary GPL-3.0. That
clashes with the Apache-2.0 Muse SDK. They migrated to an MIT driver,
`feetech_scs_esp_idf` — use that one. SCS0009 positioning timing traces back to
`stackchan-arduino` (Takao Akaki / mongonta0716).

#### Servos are serial bus, not PWM

This is the single most important finding. The official kit uses **SCS0009
feedback servos on a shared UART** (G6 TX / G7 RX) — not PWM. Most StackChan
tutorials online describe PWM SG90s on a Grove port, which **does not apply**.
StackChan's own config confirms it: `servo_type: "M5_SCS"`, and for serial
servos the x/y "pins" are unused.

- **Pan (X):** 360° continuous rotation, with feedback. No angle limit.
- **Tilt (Y):** 90° range, with feedback. Centre 90°.

> **Safety, from M5Stack's docs: keep the Y-axis within 5-85°. "Operating at
> extreme angles may cause servo stall and permanent damage."** Any servo code
> must clamp tilt before it is allowed to move at all.

So the work is an SCS0009 UART driver (position write, feedback read, ID
addressing for pan/tilt) plus a motion task, with clamping first.

#### Camera: Muse *can* see, and there is a proper extension point

An earlier note here claimed Muse cannot receive images. **That was wrong.** It
is true there is no image attachment in the *chat* path (`muse_chat.h` carries
only dictation audio and text), but images reach the agent through the Home
Link **command/tool** channel instead.

`camera.capture` is advertised to the server in `build_register_json()` with an
LLM-facing description, i.e. as a tool the agent can decide to call:

```c
add_command(commands, "camera.capture",
            "Capture one still JPEG frame from the SenseCAP Watcher camera. "
            "The frame is returned as base64 only when this command is explicitly invoked.",
            nullptr, nullptr);
cJSON_AddNumberToObject(cJSON_GetObjectItem(commands, "camera.capture"), "timeout_ms", 30000);
```

The handler in `app.c` runs the capture on its own PSRAM task and replies with
`{ "format": "jpeg-base64", "data_base64": ... }` via
`noise_ctrl_send_command_result()`. The 30 s timeout exists for that round trip.

Better still, `components/camera/camera.h` is a **board-agnostic backend
interface** built for exactly this: "Each camera is a backend that fills in a
`camera_driver_t`, and the board registers its one at startup." It offers
`camera_capture()` / `camera_release()` and optional
`camera_stream_start()` / `camera_stream_stop()` for a viewfinder. The `name`
field is commented "for people **and Muse**" — the camera is meant to be
visible to the agent.

So the work is not a protocol problem, it is a driver:

1. Write a `camera_driver_t` backend for CoreS3's **GC0308 (640x480, 0.3 MP)**,
   probably over the `espressif/m5stack_core_s3` BSP, and `camera_register()` it
   from `board_m5stack_cores3.c`.
2. Ungate the plumbing from the Watcher: `CONFIG_MUSE_WATCHER_CAMERA` guards the
   tool registration in `noise_control.cpp`, the dispatch in `app.c`, and the
   `srcs` entry in `CMakeLists.txt`. Generalise it to a board-agnostic option.
3. `watcher_camera.c` remains the reference for preview + shutter UX.

This makes the camera considerably more attractive than first assessed — "look
at this and tell me what it is" should work once the backend exists.

#### CoreS3 main-unit peripherals

All on the already-present BSP, so these are the cheap ones:

| Peripheral | Part | Notes |
|---|---|---|
| Camera | GC0308 640x480 | BSP + `watcher_camera.c` as template |
| Proximity + ambient light | LTR-553ALS-WA | internal I2C; obvious use: wake/greet on approach |
| IMU | BMI270 + BMM150 (9-axis) | tilt/shake gestures; magnets upset BMM150 |
| microSD | slot on bottom | BSP usually exposes a mount helper |

#### Suggested order within this task

1. **Proximity sensor** — smallest, self-contained, and immediately useful
   (wake and greet when someone approaches).
2. **IMU** — similar size; enables pick-up/shake reactions.
3. **SD card** — mount via BSP; useful as a sink for logs and captures.
4. **Servos** — the headline feature, and the only one that can damage
   hardware. Clamp tilt to 5-85°, prove a safe centre-and-return, then add
   idle motion and a "look at the speaker" behaviour.
5. **Camera** — write the GC0308 `camera_driver_t` backend and ungate the
   `camera.capture` tool, so Muse can ask for a frame itself.
6. **Base extras** (12 RGB LEDs, IR, Si12T touch strip, NFC) — lowest value,
   and each needs its own driver on the base I2C bus.


