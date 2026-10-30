# Bench and Bring-Up Guide

## 1. Parts Still Needed

Before beginning assembly and bench testing, acquire the following items:
- External heading LED: High-brightness LED with a current-limiting resistor or driver transistor sized for ESP32 GPIO27 (keep pin current within ESP32 limits, 12 mA recommended, 20 mA maximum).
- Power disconnect: Removable link or screw switch rated for 3S battery voltage and full motor current, installed in the battery positive lead. Must allow full power cutoff in under 15 seconds without disassembly or placing hands near rotating parts.
- Test blocks: Stable stand or blocks to support the chassis with wheels and rotating parts completely elevated and clear of any surface during pit and bench testing.

## 2. Inquiries and Event Rules

Event: KSU Combat Robotics 1lb Antweight (Ruleset v6.1 / KSU-TR-1LB-NF Rev 6.0).

### Organizer Inquiries in Writing

Send written questions to the event organizers for clarification before finalizing the build:
1. Hazard Bot pre-approval: KSU rules require written pre-approval when rotating weapon mass exceeds 50% of total robot weight, including shaft, bearing, and containment documentation. Because a melty-brain robot spins its entire body, clarify whether the entire robot is classified as a Hazard Bot.
2. Weapon lock requirement: Rules require a physical weapon lock (pin or clamp) on spinners at all times outside the arena cage. Melty robots have no separate weapon blade. Ask organizers what physical restraint (such as a transport clamp, chassis strap, or switch lock) is acceptable.

### Event Rules Compliance

| Rule | Requirement | Implementation and Status |
| --- | --- | --- |
| Weight | 1.00 lb (453.6 g) maximum for rolling robots | Chassis and components must stay under 453.6 g. Motor pair is approximately 110 g. Weigh full robot on a calibrated scale before competition. |
| Voltage | 3S LiPo allowed, 13.05 V ceiling | 3S pack voltage must not exceed 13.05 V. |
| Battery | Factory sealed, rigidly mounted, not swollen | Inspect pack condition. Mount rigidly inside chassis. |
| Radio | 2.4 GHz digital spread spectrum | Provided by FlySky FS-i6 transmitter and FS-iA6B receiver using AFHDS 2A. |
| Failsafe | Outputs cut to zero within 2.0 s of signal loss or radio off | Implemented across transmitter failsafe, firmware timeout (100 ms), and ESC signal loss cutoff. Must pass the 7-step failsafe test. |
| Disconnect | Screw switch or removable link cuts all power in under 15 s | Must be installed in positive battery lead, accessible without tool disassembly or reaching near spinning perimeter. |
| Spin-down | Dead stop within 60 s after power cutoff or failsafe | Measure on test bench and in arena. ESC center-braking at neutral assists deceleration. |
| Test stands | Powered outside arena only on blocks with wheels and weapons clear | All bench testing and pit operations must use test blocks. |
| KO | No translation within a 10 second count is a KO | Robot must achieve controlled translation to avoid a knockout. Tested during bring-up. |

## 3. FlySky FS-i6 Transmitter Setup

Configure the FS-i6 transmitter as follows:
1. Output mode: In `System Setup` -> `RX Setup`, set output format to `iBUS` (or `Serial`, depending on firmware version).
2. Aux Channels: In `Functions Setup` -> `Aux. channels`, set `Channel 5` source to `SwA`.
3. Failsafe setup: In `System Setup` -> `RX Setup` -> `Failsafe`. Menu names can vary by transmitter firmware:
   - Set `Channel 3` (spin throttle) to minimum (-100%, pulse width ~1000 us).
   - Set `Channel 5` (arm switch) to disarmed (-100%, pulse width ~1000 us).
   - Turn failsafe `On` for both channels and hold `Cancel` to save.

Channel layout and controls (Mode 2):
- CH1: Right stick horizontal (Translation X; hold full left to satisfy arming gesture).
- CH2: Right stick vertical (Translation Y).
- CH3: Left stick vertical (Spin throttle; hold full down to satisfy arming gesture).
- CH4: Left stick horizontal (Heading trim rate).
- CH5: Switch SwA (Disarmed = low/down; Armed = high/up).

### Arming Sequence

The microcontroller board is a confirmed classic ESP32 (ESP32-D0WDQ6-V3). Arming requires:
1. Left stick held full down (CH3 <= 1050 us).
2. Right stick held full left (CH1 <= 1050 us).
3. Flip switch SwA from disarmed to armed (CH5 transitions from <= 1700 us to > 1700 us).
4. Sensor startup check must have passed at boot.

If SwA was already armed at power-on, or if any failsafe tripped, flipping SwA directly will not re-arm. Move SwA back to disarmed, satisfy both stick positions, and flip SwA to armed again.

## 4. iBUS Voltage Check and Voltage Divider

The ESP32 GPIO pins are not 5 V tolerant and must not receive more than 3.3 V. FS-iA6B receiver iBUS signal voltage varies between hardware revisions.

1. Power the receiver from a 5 V supply (or receiver battery) with signal disconnected from the ESP32.
2. Using an oscilloscope, measure the peak signal voltage on the receiver iBUS pin relative to ground. A multimeter shows an average, not the peak, so it cannot clear this check. Without a scope, use the divider.
3. If measured peak voltage is above 3.3 V, or was not measured with a scope:
   - Build a resistor voltage divider between receiver iBUS out and ESP32 GPIO16:
     - 1.0 kOhm resistor between receiver iBUS pin and ESP32 GPIO16.
     - 2.0 kOhm resistor between ESP32 GPIO16 and ground.
   - This scales a 5 V signal down to approximately 3.3 V.
4. If the scope shows 3.3 V or lower, the receiver pin may connect directly to GPIO16.
5. After wiring, confirm channels read cleanly at 115200 baud, using Step 1 of section 7.

## 5. Wiring and Power

### Warning: Power Conflicts
Do not connect USB and the ESC's BEC at the same time unless the board's USB power path has been checked and verified to prevent back-feeding. When connecting USB for console configuration, disconnect the BEC 5 V line to the ESP32 5 V pin, or power the ESP32 solely via USB with the battery disconnected.

### ESC Mixing Jumper
Leave the Repeat Robotics dual ESC mixing jumper uncut. Mixing must remain disabled so each signal pin directly controls one individual motor.

### Wiring Connections

| Signal | ESP32 Pin | Destination | Notes |
| --- | --- | --- | --- |
| iBUS in | GPIO16 (UART2 RX) | Receiver iBUS pin | Pass through 1k/2k voltage divider if receiver output exceeds 3.3 V. |
| ESC Motor A | GPIO25 | Repeat ESC input A | 3-pin connector. Carries BEC 5 V, ground, and signal A. |
| ESC Motor B | GPIO26 | Repeat ESC input B | 1-pin connector (signal B). |
| Sensor SDA | GPIO21 | GY-521 MPU-6050 SDA | I2C data (400 kHz). |
| Sensor SCL | GPIO22 | GY-521 MPU-6050 SCL | I2C clock (400 kHz). |
| Heading LED | GPIO27 | LED anode / driver | Current-limited by series resistor. |
| +5 V Power | ESP32 5V pin | ESC BEC 5V, GY-521 VCC, and Receiver VCC | Powered by ESC 5V 1.5A BEC. |
| Ground | ESP32 GND | Common ground | ESC GND, Receiver GND, GY-521 GND, and LED cathode must share common ground. |
| Main Battery | Battery (+) via disconnect | ESC Battery (+) | 3S LiPo through removable link or screw switch. |
| Main Battery | Battery (-) | ESC Battery (-) | Negative battery lead directly to ESC. |

## 6. Motor Winding Resistance and Stall Protection

The Repeat Robotics dual brushed ESC is listed at 2 A continuous and 4 A burst. The listing does not say whether that is per channel or total, so treat it as per channel at most. Gecko motors can draw high stall currents that exceed ESC ratings if jammed.

1. Disconnect motor leads from the ESC.
2. Using a multimeter, measure winding resistance (R) across the motor terminals. Subtract probe lead resistance.
3. Estimate maximum stall current at full charge (12.6 V):
   `stall_amps = 12.6 / R`
4. Calculate the stall power cap:
   `cap = 2.0 / stall_amps`
5. Connect to the ESP32 serial console (115200 baud on COM6, for example `python -m platformio device monitor`).
6. Set `stallCap` to a value no higher than `cap`:
   `set stallCap <value>`
   `save`
   Example: If R is 1.0 Ohm, `stall_amps = 12.6 A` and `cap = 2.0 / 12.6 = 0.159`. Set `stallCap` no higher than 0.159. This is an estimate; check current with a clamp meter or inline meter if available.

## 7. Bring-Up and Test Sequence

All steps must be conducted with the robot elevated on test blocks and wheels completely free of contact with surfaces. Keep fingers clear of wheels and rotating parts.

### Serial Console Operation
- Port: COM6 at 115200 baud (for example `python -m platformio device monitor`).
- Console commands work only while disarmed.
- Available commands:
  - `status`: Toggle live 200 ms status output.
  - `set <name> <value>`: Modify parameter in RAM.
  - `show`: Display current configuration.
  - `save`: Save settings to NVS flash.
  - `defaults`: Reset parameters in RAM to defaults (requires `save` to persist).
  - `log`: Output CSV telemetry from RAM if present, otherwise from the saved run in LittleFS. It works after a power cycle. Each new disarm with a nonempty log overwrites the saved run. A storage failure is reported by `storage=` in status or by `log`; RAM logging and robot control continue.
- Notice: `outputHz` needs `save` plus a reboot to take effect.
- Notice: Heading and LED alignment and all default settings are unverified starting values.

### Step 1: iBUS Read Verification
1. Power receiver and ESP32 with wheels on blocks. Disarm transmitter (SwA down).
2. Open serial monitor. Verify the status line displays:
   `CH1=... CH2=... CH3=... CH4=... CH5=... signal=ok ...`
3. Check channel responses:
   - Right stick full right: CH1 2000 us.
   - Right stick forward: CH2 2000 us.
   - Left stick up: CH3 2000 us.
   - Left stick right: CH4 2000 us.
   - SwA armed: CH5 2000 us.
   - If any is backwards, flip it in the i6 Reverse menu.
4. If signal shows `lost` or values do not update, check iBUS wiring and transmitter output mode.

### Step 2: Sensor Setup and Heading Calibration
1. Mount MPU-6050 approximately 10 mm from the chassis spin center.
2. Measure physical distance from the center of rotation to the accelerometer chip package in meters.
3. Power the ESP32 and open serial monitor. Confirm `startup=ok` and `sensor=live` appear in the status stream.
4. Hold the robot on edge, with the sensor directly below the spin center, so the direction from sensor to center points straight up. `radial` should read close to +1.0:
   - If `radial` is near zero, switch axis: `set radialAxis 1` followed by `save`, then check again.
   - If `radial` is close to -1.0, set `radialSign -1` followed by `save`.
   - Spinning by hand is too slow to give a clear reading at this radius, so use this tilt check instead.
5. Invert the robot by hand:
   - Status output should display `flipped=1` when upside down and `flipped=0` when upright.
   - If inverted state is opposite, toggle `zSign -1` followed by `save`.
6. Set measured sensor radius:
   `set radiusM <meters>`
   `save`
7. Heading calibration:
   - If RPM or the LED drifts, adjust `radiusM` in small increments.
   - Use `phaseRad` to align drive direction:
     `set phaseRad <radians>`
     `save`

### Step 3: ESC Signal Verification on Test Blocks
1. Ensure chassis is securely supported on test blocks.
2. Connect ESC signal lines (GPIO25 to Motor A, GPIO26 to Motor B).
3. Power on. Verify motors stay completely stationary while disarmed. Output uses one-shot RMT pulses at the boot-time `outputHz`; if the CPU or loop stops, pulses stop. Verify ESC signal-loss stopping in failsafe step 5. On disarm, neutral replaces any pending command and is retried without waiting when an in-flight pulse finishes (at most about 2 ms).
4. Perform arming sequence: Left stick full down, right stick full left, flip SwA to armed.
5. Slowly advance CH3 throttle. Verify both motors spin in directions that produce forward robot rotation:
   - If Motor A spins in reverse, disarm and enter: `set reverseA 1`, then `save`.
   - If Motor B spins in reverse, disarm and enter: `set reverseB 1`, then `save`.
6. Disarm (flip SwA down). Verify motors stop immediately.

### Step 4: The 7-Step Failsafe Test
Conduct this exact 7-step test on test blocks before any free-running operation:
1. Power on with SwA armed, then raise CH3: motors must stay still.
2. Arm and spin slowly, then turn off the transmitter: motors must get zero output within about one second. The rule limit is 2.0 s.
   Notice: If failsafe test step 2 fails, the robot doesn't fight until fixed. The FS-iA6B's iBUS behavior on signal loss is unconfirmed.
3. Turn the transmitter back on with SwA still armed, then raise CH3: motors must stay stopped until the full arming sequence is repeated.
4. Arm and spin slowly, then unplug iBUS: motors must stop.
5. Arm and spin slowly, then hold the ESP32 in reset: motors must stop.
6. On blocks, with `status` on and disarmed, unplug the sensor and confirm `sensor=stale` replaces `sensor=live` after 500 ms. Reboot with the sensor unplugged: `startup=failed` must appear and arming must be refused. Reconnect the sensor and reboot; then arm, spin slowly, and unplug it: the LED must stop and disarming must still stop the motors. Translation cutoff is covered by the host sequence test; blocks cannot demonstrate it.
7. Power cycle with the ESC connected several times: motors must not twitch at any point.

### Step 5: Jam Protection Test
Never touch or hold a running wheel. The chassis does not rotate on blocks, so measured RPM stays near 0 and the firmware sees a jam by itself. That makes this test possible with free wheels.

1. With wheels on blocks, clear of everything, and `stallCap` configured, arm the robot.
2. Raise CH3 above the `stallCap` level, then hold it for at least 3 seconds.
3. Disarm and allow the saved run write to complete before removing power. Disconnect BEC power before connecting USB as described in section 5, then run `log`, even after a power cycle. Verify `aUs` and `bUs` level off at the `stallCap` output about 1 second after power passes the cap. For example, a cap of 0.3 gives 1650 us, or 1350 us on a reversed motor.
4. Expect this limit in all testing on blocks, since RPM stays below `minRpm`.

### Step 6: Spin-Up Verification
1. Conduct inside a fully enclosed test arena or containment box.
2. With robot on the floor inside the enclosure, arm and gradually raise CH3 throttle.
3. Verify smooth acceleration without sudden jerks or ESC thermal shutdown.
4. Check LED flashing behavior once robot exceeds `minRpm`.
5. A slow spin-up can be held at the stall cap below `minRpm`, so set `stallCap` and `minRpm` from measured spin-up behavior. If the robot stalls at low speed and never reaches `minRpm`, the stall cap is holding it down. Lower `minRpm` with `set minRpm <rpm>` and `save` before raising `stallCap`, and do not raise `stallCap` above the section 6 value.

### Step 7: Flipped Driving Test
1. Place robot inverted inside the test arena.
2. Arm and spin up.
3. Command translation using right stick (CH1 and CH2).
4. Verify translation direction and LED orientation match driver stick perspective without inverted controls.

### Step 8: Spin-Down Time Under 60 Seconds
1. Spin robot up to full operating speed inside the arena.
2. Flip SwA to disarm or switch off transmitter to trigger failsafe.
3. Start a stopwatch at the moment of cutoff.
4. Measure elapsed time until robot reaches a dead stop.
5. Verify total deceleration time is under 60 seconds to satisfy KSU rules.

## 8. What to Send Back After Each Stage

Provide the following test artifacts and measurements after completing each stage:

1. Stage 1 (Radio and iBUS):
   - Copy of status line at center sticks.
   - Verified pulse width extremes for CH1 through CH5 (stick limits and SwA).
2. Stage 2 (Sensor setup):
   - Status line at rest and during slow hand spin.
   - Selected values for `radialAxis`, `radialSign`, `zSign`, and measured `radiusM`.
3. Stage 3 (Motor and ESC):
   - Motor winding resistance measurement in Ohms.
   - Calculated stall current and selected `stallCap`.
   - Settings for `reverseA` and `reverseB`.
4. Stage 4 (Failsafe 7-step test):
   - Explicit Pass/Fail confirmation for each step from 1 through 7.
   - Measured time to zero output on transmitter switch-off in step 2.
5. Stages 5 to 8 (Dynamics, Jam, Spin, Inversion, Spin-down):
   - Disarm and allow the saved run write to complete, disconnect BEC power per section 5, connect USB, and execute `log`. The saved run survives reboot; each new nonempty disarm overwrites it.
   - Send captured CSV telemetry log (`ms,rpm,radialG,aUs,bUs`).
   - Timed spin-down duration in seconds.
