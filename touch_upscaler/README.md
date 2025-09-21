| State       | Meaning                                                                                  |
| ----------- | ---------------------------------------------------------------------------------------- |
| **Idle**    | No touch detected. Waiting for a valid sample.                                           |
| **Active**  | Touch is ongoing. Sending interpolated/upscaled Move events.                             |
| **MaybeUp** | Sensor reported invalid/missing sample; touch may have lifted. Waiting for grace period. |



| Event / Condition   | Description                                       |
| ------------------- | ------------------------------------------------- |
| `ValidSample`       | IR module reports a valid sample.                 |
| `InvalidSample`     | IR module reports invalid sample.                 |
| `IdleGraceExceeded` | Time since last valid sample exceeds `idleGrace`. |
| `TouchDownDetected` | First valid sample received while in Idle.        |
| `TouchUpConfirmed`  | Touch is confirmed lifted (after MaybeUp grace).  |



| Current State | Event / Condition            | Next State | Action                                    |
| ------------- | ---------------------------- | ---------- | ----------------------------------------- |
| Idle          | ValidSample                  | Active     | Emit Down event, add sample to history    |
| Idle          | InvalidSample                | Idle       | Nothing                                   |
| Active        | ValidSample                  | Active     | Interpolate / upscale, emit Move event    |
| Active        | InvalidSample                | MaybeUp    | Start MaybeUp timer (grace period)        |
| Active        | IdleGraceExceeded            | Idle       | Emit Up event, clear history              |
| MaybeUp       | ValidSample                  | Active     | Resume Move events, cancel MaybeUp timer  |
| MaybeUp       | InvalidSample (within grace) | MaybeUp    | Hold last output (optional extrapolation) |
| MaybeUp       | IdleGraceExceeded            | Idle       | Emit Up event, clear history              |




          +-------+
          | Idle  |
          +-------+
            |
      ValidSample (TouchDownDetected)
            v
          +--------+
          | Active |
          +--------+
         /          \
ValidSample         InvalidSample
  |                   |
  v                   v
Active              +---------+
                    | MaybeUp |
                    +---------+
                   /           \
          ValidSample       IdleGraceExceeded
               |                  |
               v                  v
             Active              Idle

Key Notes for Run Loop Coding

History Buffer (vector<TouchPoint>): keep last N valid samples for interpolation/extrapolation.

mLastOutput: stores last emitted point (for EMA smoothing).

IdleGraceExceeded: computed as currentTime - lastValidSampleTime > idleGrace.

MaybeUp Timer: track how long we’ve been in MaybeUp; if it exceeds idleGrace, emit Up.

Worker Thread Run Loop:

Runs at outputHz (e.g., 130 Hz).

Reads last samples from history.

Checks current state and applies FSM logic to emit Down/Move/Up.

Applies interpolation/extrapolation as needed.