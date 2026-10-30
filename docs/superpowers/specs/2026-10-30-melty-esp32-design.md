# Melty ESP32 firmware design

Date: 2026-10-30

## Intent

Firmware and wiring for a melty brain combat robot going to a real event. The robot spins as a whole and drives by timing a power change to each motor once per rotation. An ESP32 sits between the receiver and the ESC to do that timing.

The owner is using parts already on hand and will not buy brushless parts. CAD and the chassis are out of scope.

## Event

KSU Combat Robotics 1lb Antweight, February 27, 2027, Kent State University. Checked against KSU-TR-1LB-NF Rev 6.0, the text of the PDF linked as Ruleset v6.1.

| Rule | Requirement | Status in this design |
| --- | --- | --- |
| Weight | 1.00 lb (453.6 g) for rolling robots | Owner's chassis. The two motors alone are about 110 g |
| Voltage | 3S LiPo allowed, 13.05V ceiling | Met by the 3S pack |
| Battery | Factory sealed, rigid mounted, not swollen | Owner's chassis |
| Radio | 2.4 GHz digital spread spectrum | Met by the i6 |
| Failsafe | Outputs at zero within 2.0 s of signal loss or radio off | Layers below, proven by the failsafe test |
| Disconnect | Screw switch or removable link fully cuts all power in under 15 s, no disassembly, no limbs near weapons | Must be added, see Hardware |
| Spin-down | Dead stop in under 60 s after cutoff or failsafe | Measure in testing |
| Test stands | Powered outside the arena only on blocks, wheels and weapons clear of all surfaces | Follow in the pits. Bring test blocks |
| KO | No translation within a 10 second count is a KO | Translation is required to stay in a match, see Risks |

Two rules need a written answer from the organizers, see Risks: Hazard Bot pre-approval and weapon locks.

## Goals

- Spin up, translate in the commanded direction, and show the driver a heading from an LED, right side up or flipped.
- Stop the motors reliably on every failure this design can detect, and give the driver a repeatable failsafe setup and test.
- Allow tuning over USB between runs without recompiling.

## Non-goals

- CAD, chassis, or wheel selection.
- Brushless support.
- WiFi or Bluetooth control, tuning, or telemetry. Both radios stay off.
- Absolute heading sensing. The driver trims heading by eye.
- Driving as a normal non-spinning two-wheel robot.

## Anti-goals

These count as failures even if the robot otherwise works:

- Motors move at power-up, on reboot, or while disarmed.
- Motors keep running after the transmitter is off.
- USB tuning can change anything while armed.

## Hardware

| Part | Detail | Source |
| --- | --- | --- |
| MCU | ESP32 board on COM6 with a CP210x USB chip. Assumed classic ESP32, chip not yet confirmed | Device manager |
| Radio | FlySky FS-i6 with FS-iA6B receiver, using iBUS | Owner |
| Motors | 2x DartBox V2 Gecko, brushed, 22.5:1, 1100 +/- 55 RPM at 12V/3S, about 54 to 56 g each | Seller listing |
| ESC | Repeat Robotics dual brushed ESC, details below | Seller listing |
| Sensor | GY-521 MPU-6050 module, HiLetgo. Accelerometer only, at +/-16g | Owner |
| Battery | 3S | Owner |
| LED | External bright LED and resistor. Not yet owned | Design |
| Disconnect | Screw switch or removable link in the battery positive lead. Not yet owned | KSU rules |

ESC details from the listing:

- Accepts RC PWM, reverse at 800 to 1100 us and forward at 1900 to 2200 us, with auto-adjust.
- Center braking.
- Mixing is off by default and enabled by cutting a jumper. Leave it uncut, so each signal drives one motor.
- 2A continuous, 4A burst.
- 5V 1.5A BEC on the 3 pin connector.
- 2S to 3S recommended.
- Stops the motors when signal is lost.
- Maximum signal update rate and response time are not published.

## Wiring

Pin numbers assume a classic ESP32. They avoid strapping pins so ESC signals stay quiet during boot.

| Signal | ESP32 pin | Notes |
| --- | --- | --- |
| iBUS in | GPIO16 (UART2 RX) | Through a divider unless measured at 3.3V or lower, see below |
| ESC motor A | GPIO25 | 3 pin ESC connector |
| ESC motor B | GPIO26 | 1 pin ESC connector |
| MPU-6050 SDA | GPIO21 | I2C 400 kHz |
| MPU-6050 SCL | GPIO22 | I2C 400 kHz |
| Heading LED | GPIO27 | Through a resistor or transistor sized for the LED, within the pin's current limit |

Power: the ESC BEC feeds 5V to the ESP32 5V pin, the receiver, and the GY-521 VCC. All grounds are common. Do not power the board from USB and the BEC at once unless the board's USB power path is checked first. The ESP32 and sensor run from their own regulators.

iBUS voltage: sources conflict, with reports of both 5.0V and 3.3V, and an ESP32 pin must not see more than 3.3V. Measure before connecting. Unless measured at 3.3V or lower, use a 1k and 2k divider. Confirm 115200 baud frames still read cleanly through it.

ESC signal voltage: the ESP32 outputs 3.3V. No listing states the ESC's input threshold, so this is confirmed by the bench test.

## Radio control

The i6 stock firmware sends 6 channels.

| Control | Channel | Purpose |
| --- | --- | --- |
| Right stick horizontal | CH1 | Translation X |
| Right stick vertical | CH2 | Translation Y |
| Left stick vertical | CH3 | Spin speed |
| Left stick horizontal | CH4 | Heading trim, rotates the LED and drive direction at a rate set by stick deflection |
| SwA | CH5 | Arm and disarm |

The i6 needs SwA assigned to CH5 in its Aux Channels menu, and iBUS output selected in its RX Setup menu (the setting there is named Serial or iBUS depending on transmitter firmware).

## Behavior

### Sensing

Read radial acceleration from the MPU-6050 at +/-16g. Spin rate is `omega = sqrt(a / r)`, where `r` is the calibrated sensor radius. Heading is the running integral of `omega`.

Mount the sensor about 10 mm from the spin center. At that radius 16g is reached at about 1200 RPM. Expected spin is around 550 RPM with 2 inch wheels 2 inches from center, an estimate based on assumed dimensions. Mount radius and axis are settings.

If a reading clips or is implausible, keep the last good rate and recover on the next good reading. If the sensor fails for 500 ms while armed, keep spin but stop translation and turn the LED off, so the driver can see it and disarm.

### Motor output

Motors only ever run forward during spin. Output pulses stay between neutral and full forward and never cross into reverse, because center braking would fight the spin. A per-motor setting flips direction so the two motors can be wired either way.

- Spin base power comes from CH3.
- Spin-up is rate limited with a slow ramp, to limit stall current into a 4A burst ESC.
- Below a minimum measured RPM, translation stays off.
- For translation, each motor gets more power for the half rotation that pushes the robot in the commanded direction and less for the opposite half, by an amount set by stick deflection. Output stays in the forward range.

Start with a 50 Hz signal, then raise it based on the bench test. At 550 RPM a rotation takes about 109 ms, which is 5 updates per rotation at 50 Hz.

### Jam protection

If commanded power is above the stall cap and measured RPM stays below the minimum RPM for 1 second, motor power is cut to the stall cap until RPM recovers. The cap is a tunable setting, set from the stall current measurement so it stays within the ESC rating. The driver accepted the loss of pushing power while pinned. This protects a jam or pin, not the first moments of spin-up, which is what the ramp is for.

### Running flipped

The robot must drive inverted. The chassis must allow it, with wheels reaching the floor from both sides.

Read the sensor's Z axis, which sees gravity while the robot spins. A slow filter ignores impacts and hides brief tilt. When the sign changes, the firmware mirrors translation direction and LED position, so stick directions stay the same from the driver's view. The Z axis sign for right side up and the filter time are settings.

### LED

Flash a short arc at the forward heading once per rotation.

### Tuning

USB serial on COM6 at 115200. Commands are accepted only while disarmed. Settings persist in NVS, so they survive reboot. Tunable values include sensor radius, trim rate, pulse limits, ramp rate, minimum RPM, translation strength, phase offset, motor directions, and output rate.

Serial shows a live status line while disarmed, with channels, failsafe state, acceleration, and estimated RPM, for setup and testing.

A spinning robot cannot stay on USB, so the firmware records a short run log to RAM while armed, with time, estimated RPM, sensor reading, and motor output. On disarm, a nonempty log is saved to LittleFS after neutral output is accepted. Each such disarm overwrites the saved run. It is dumped over USB after disarming or a power cycle, preferring RAM entries when present. The bench test reads ESC response from this log, so no scope is required.

## Failsafes

Motors stop when any failsafe trips. There are three independent layers.

### Layer 1: transmitter and receiver, set by the driver

In the i6 RX Setup, Failsafe menu, set:

- CH5 (arm) to the disarmed position.
- CH3 (spin) to minimum.

This layer matters because the receiver may keep sending normal-looking iBUS frames after the transmitter is lost, so the ESP32 may not be able to detect the loss from frame timing alone. How the FS-iA6B behaves over iBUS on signal loss is unconfirmed. Sources conflict: one report shows a failsafe flag in a channel's high byte, and another says failsafe works only on PWM. The failsafe test below settles it on this hardware. If the test fails, the robot does not fight until it passes.

### Layer 2: ESP32 firmware

Motors go to neutral, then output is held at neutral until the driver goes through the arming sequence again, when any of these occur:

- No valid iBUS frame for 100 ms.
- Any used channel is out of range or has the failsafe flag set.
- CH5 is in the disarmed position.

A failsafe can look the same as a real switch flip: the receiver forces CH5 to disarmed, then CH5 snaps back to armed when the transmitter returns. So moving SwA alone never arms the robot.

Arming requires all of these at the moment CH5 moves from disarmed to armed:

- Valid iBUS frames.
- CH3 at minimum.
- CH1 held at full left. The stick springs back to center, so a returning transmitter cannot satisfy this by itself.
- Sensor passed its startup check.

If CH5 moves to armed without these, the robot stays disarmed until CH5 returns to disarmed and the sequence is repeated. A switch left armed at boot is handled the same way.

The startup check confirms the MPU-6050 answers on I2C and returns plausible readings at rest. Clone chips can report an unexpected WHO_AM_I value, so identity is checked by behavior rather than by one exact ID.

Other rules:

- The ESC gets a steady 1500 us neutral from firmware start, while disarmed, and after any failsafe. Both ESC pins are set to neutral before the control loop starts.
- Output is clamped to 1500 to 2000 us. The bench test must show that the ESC's auto-adjust does not shift neutral or range when it only ever sees this band.
- A watchdog resets the ESP32 if the control loop stalls. After reset, the robot is disarmed.
- I2C and UART handling must never block the control loop.
- Sensor failure while armed is the one exception to stopping. It stops translation and turns the LED off, but keeps spin so the driver keeps control of when to stop.

### Layer 3: ESC

The ESC stops motors when its signal is lost, per the listing, which covers an ESP32 crash that leaves the output pins silent. Confirmed by the failsafe test.

### Failsafe test

Run with wheels off the ground on test blocks, on a current-limited supply if available, and repeat after any firmware or radio change, and before each event:

1. Power on with SwA armed, then raise CH3: motors must stay still.
2. Arm and spin slowly, then turn off the transmitter: motors must get zero output within about one second. The rule limit is 2.0 s.
3. Turn the transmitter back on with SwA still armed, then raise CH3: motors must stay stopped until the full arming sequence is repeated.
4. Arm and spin slowly, then unplug iBUS: motors must stop.
5. Arm and spin slowly, then hold the ESP32 in reset: motors must stop.
6. On blocks, with `status` on and disarmed, unplug the sensor and confirm the live sensor state goes stale after 500 ms. Reboot with the sensor unplugged and confirm arming is refused. Reconnect and reboot, then arm and spin slowly before unplugging the sensor: the LED must stop and disarming must still stop the motors. Verify translation cutoff with the host good-samples-to-NAN sequence, since blocks cannot show it.
7. Power cycle with the ESC connected several times: motors must not twitch at any point.

## Risks

- **ESC current:** the ESC is rated 2A continuous and 4A burst. Gecko stall current is not published, and the seller warns stalling can quickly destroy the motors and calls current-limiting ESCs a must. A spin-up from rest and a jammed wheel are both stall conditions. The ramp reduces this but cannot remove it. Measure motor winding resistance and compute stall current at 12V before spinning at full power. The ESC is the most likely part to fail.
- **Hit strength:** estimated spin around 550 RPM and about 1 joule stored in a 1 lb robot, using assumed dimensions. Accepted by the owner.
- **ESC response:** if it does not respond fast enough at this spin rate, translation will be weak or absent. Unknown until the bench test.
- **Sensor:** the MPU-6050 and its module may not survive hits. At 10 mm from center, vibration and impacts can also corrupt readings.
- **Hazard Bot status:** KSU requires written pre-approval when spinning weapon mass exceeds 50% of robot weight, with shaft, bearing, and FEA or burst containment documentation. A melty's whole body spins, so it may count. Ask the organizers in writing before building further around this event.
- **Weapon locks:** KSU requires a physical pin or clamp on spinners at all times outside the cage, and rules out powered testing without it. There is no separate weapon on a melty to pin. Ask the organizers what they will accept, such as a chassis strap or disarm plus disconnect.
- **KO rule:** a robot with no translation for a 10 second count is KO. If the sensor dies or the ESC response is too slow, the robot spins but cannot translate. There is no fallback drive mode in this design.
- **Braking at neutral:** failsafe sends the ESC to neutral, which brakes. That helps the 60 s spin-down rule, but braking from full speed may also stress a 4A ESC. Check during spin-down tests.

## Testing

- Host-run unit tests for the iBUS parser, the failsafe and arming logic, and the heading calculation.
- Bench tests in order: iBUS read, sensor read, ESC signal and response, failsafe test, then spin-up with the robot secured.
- Bench and arena tests in order: jam protection, flipped driving, spin-down time under 60 s.

## Decided by Claude, not yet reviewed by the owner in detail

- PlatformIO with the Arduino framework, in C++.
- GPIO assignments.
- The 100 ms iBUS timeout and 500 ms sensor timeout.
- Keeping spin without translation on sensor failure.
- Arming gesture of SwA plus CH1 full left.
- Sending 1500 us neutral at all times, rather than no signal, while disarmed.
- Run log to RAM.
- Automatic flip detection from the Z axis, instead of a switch on CH6.
- Jam protection trigger of 1 second below minimum RPM.

## Left to the builder

- Control loop structure, core pinning, and exact task layout.
- MPU-6050 filter and register settings.
- USB command format.
- Whether to use an iBUS library or parse frames directly.
- Choice of ESP32 PWM peripheral.

## References

- [Repeat dual brushed ESC listing](https://palmbeachbots.com/products/repeat-robotics-dual-brushed-esc-for-ant-drive)
- [Gecko 4mm listing](https://palmbeachbots.com/products/just-cuz-dartbox-v2-gecko-drive-gear-motor-4mm-shaft)
- [KSU 1lb event](https://www.ksucombat.club/events/1lb)
- [KSU 1lb ruleset PDF](https://igsojmdo4jbyfnpr.public.blob.vercel-storage.com/documents/ksu_combat_1lb_ruleset_v6.1-51AuSZMQnDpIX5CEjW4UsfDzvHkR8d.pdf)
- [Open Melt](https://github.com/nothinglabs/openmelt2/)
- [IBusBM issue on failsafe flag](https://github.com/bmellink/IBusBM/issues/24)
- [Deviation forum thread on AFHDS 2A](https://www.deviationtx.com/forum/protocol-development/5251-flysky-afhds-2a-protocol-as-used-i10-i6-it4?start=400)
