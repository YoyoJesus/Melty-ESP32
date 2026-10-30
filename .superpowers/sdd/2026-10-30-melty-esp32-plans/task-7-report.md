# Task 7 report

Status: DONE_WITH_CONCERNS

Implemented ESP32 ESC, MPU-6050 and Preferences drivers and the control loop, using the existing melty APIs. No lib/melty changes. The output rate can be changed while disarmed; the startup sensor check gates arming, and the task watchdog is configured for 250 ms with panic.

Verification:
- `python -m platformio run -e esp32 -t upload`: SUCCESS on COM6, final build uploaded and verified.
- `wsl -- bash -lc "cd /mnt/c/users/austin/onedrive/documents/github/Melty-ESP32 && PLATFORMIO_BUILD_DIR=/tmp/melty-pio ~/.venvs/pio/bin/pio test -e native"`: 53/53 passed.
- `rg 'WiFi|BluetoothSerial' src lib`: no matches.
- `git diff --check`: passed.

COM6 serial evidence from the initial upload (the final upload also booted and showed the persisted value):
```text
E (52) task_wdt: esp_task_wdt_init(517): TWDT already initialized
Melty ESP32 boot
CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
COMMAND: status
COMMAND: set radiusM 0.02
COMMAND: save
Saved
COMMAND: set radiusM 999
Rejected
COMMAND: show
radiusM=0.02
RESET
Melty ESP32 boot
CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
radiusM=0.02
```
The test setting was restored to defaults and saved after verification. The watchdog init reports already initialized on this framework, then reconfiguration succeeds; the boot log contains that diagnostic. Motor, sensor, and iBUS paths could not be exercised with nothing wired.

Files: src/main.cpp, src/esc.h, src/esc.cpp, src/mpu.h, src/mpu.cpp, src/store.h, src/store.cpp.

Self-review concerns: the hardware paths and watchdog reset under a stalled loop are not verified. In particular, console output while disarmed can delay processing queued iBUS data; frames are timestamped when processed, so queued frames may be treated as fresh. This needs safety review and a receiver-connected test before using the firmware on a spinning robot. ESC neutral and timing need measurement on real hardware. No tests were added for the hardware integration.

## Review fixes

Discard queued iBUS bytes and reset the parser after a loop gap over 20 ms. Initialize ESC frequency once from stored settings; changes require save and reboot. Move Wire reads to a core 0 task with a synchronized latest sample, use only fresh good samples for heading, and retain the last good Z for flip detection. Disable ESP driver logging and query watchdog initialization before init or reconfiguration.

Verification commands and output:
- `$env:PLATFORMIO_BUILD_DIR = 'C:/tmp/melty-pio'; python -m platformio run -e esp32`: SUCCESS.
- `$env:PLATFORMIO_BUILD_DIR = 'C:/tmp/melty-pio'; python -m platformio run -e esp32 -t upload`: SUCCESS, COM6, flash hash verified.
- `wsl -- bash -lc "cd /mnt/c/users/austin/onedrive/documents/github/Melty-ESP32 && PLATFORMIO_BUILD_DIR=/tmp/melty-pio ~/.venvs/pio/bin/pio test -e native"`: 53 test cases, 53 succeeded.
- COM6 at 115200 baud: reset using RTS, capture for 13 seconds, send `set radiusM 999`, `set outputHz 100`, and `defaults`. Full timed output follows. No TWDT already initialized message or subsequent reset appeared. Changes were not saved.

```text
0.0s ets Jul 29 2019 12:21:46
0.0s
0.0s rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
0.0s configsip: 0, SPIWP:0xee
0.0s clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
0.0s mode:DIO, clock div:2
0.0s load:0x3fff0030,len:4640
0.0s load:0x40078000,len:15632
0.0s load:0x40080400,len:3164
0.0s entry 0x4008059c
0.2s Melty ESP32 boot
0.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
0.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
0.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
1.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
1.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
1.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
1.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
1.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
2.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
2.0s COMMAND: set radiusM 999
2.1s Rejected
2.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
2.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
2.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
2.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
3.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
3.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
3.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
3.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
3.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
4.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
4.0s COMMAND: set outputHz 100
4.1s Save and reboot to apply outputHz
4.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
4.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
4.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
4.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
5.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
5.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
5.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
5.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
5.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
6.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
6.0s COMMAND: defaults
6.1s Save and reboot to apply outputHz
6.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
6.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
6.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
6.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
7.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
7.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
7.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
7.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
7.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
8.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
8.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
8.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
8.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
8.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
9.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
9.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
9.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
9.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
9.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
10.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
10.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
10.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
10.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
10.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
11.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
11.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
11.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
11.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
11.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
12.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
12.2s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
12.4s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
12.6s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
12.8s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
13.0s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost sensor=failed radial=nan rpm=0 flipped=0
```

Without wiring, receiver backlog and arming, actual sensor samples and a stuck bus, motor pulse neutrality and timing, and armed watchdog behavior could not be exercised. Hardware safety validation remains necessary before operating the robot.

## Final review fixes, partial

Status: BLOCKED. Finding 2 was implemented: zero spin returns neutral and translation is capped by spin. Finding 9 gained the requested frame-after-gap regression test. Findings 1 and 3 through 8, the guide addition, flashing, and serial verification remain unfinished.

The installed Arduino `esp32-hal-rmt.c` returns false from `rmtWriteAsync` when the previous transmission is pending. This leaves the required immediate neutral transmission on disarm unresolved when disarm occurs during a pulse; no unverified motor-output change was made. Armed operation, RMT pulses, and LittleFS run-write were not tested without wiring.

Commands and output:
- `wsl -- bash -lc "cd /mnt/c/users/austin/onedrive/documents/github/Melty-ESP32 && PLATFORMIO_BUILD_DIR=/tmp/melty-pio ~/.venvs/pio/bin/pio test -e native -f test_drive"`: before the fix, zero-spin expected 1500 but got 1600, and low-spin limit failed; the process also reported SIGINT.
- `wsl -- bash -lc "cd /mnt/c/users/austin/onedrive/documents/github/Melty-ESP32 && PLATFORMIO_BUILD_DIR=/tmp/melty-pio ~/.venvs/pio/bin/pio test -e native"`: 55 test cases, 55 succeeded.
- `python -m serial.tools.list_ports -v`: COM6 detected as Silicon Labs CP210x.

## Final review fixes, follow-up

Status: incomplete. Findings 2 and 9 were already in commit 7b1a885. Updated the guide for finding 4 (continuous current estimate), finding 8 (directional endpoints), and the spin-up guide addition. Findings 1, 3, 5, 6, and 7 remain unfinished. The guide references for findings 5 through 7 remain unchanged until the corresponding firmware is implemented. No firmware was flashed and no serial capture was made. Armed operation, RMT pulses, and LittleFS run-write remain untested without wiring. Do not operate the robot with the current firmware.

Commands and output:
- `git status --short --branch`: clean, branch feat/melty-firmware ahead 1.
- `git log -1 --oneline`: `7b1a885 fix: limit translation at low spin`.
- `git diff --check`: passed after guide edits.
- Host suite, firmware build, and COM6 verification were not run in this follow-up. No commit was made.

## Final review fixes completed, 2026-10-30

This section supersedes the incomplete statuses above. Status: DONE_WITH_CONCERNS. Stayed on feat/melty-firmware from 7b1a885. No push and no test-only arming path.

Changes by finding:

1. Replaced LEDC with 1 MHz one-shot Arduino RMT output on GPIO25/26. Initialization sends no pulses. The loop schedules both armed and disarmed output at the boot-only outputHz, retries rejected channels on the next pass, and always substitutes the latest wanted widths. Accepted channels are remembered during a retry so differing completion times cannot cause duplicate pulses or prevent neutral completion. Two buffers per channel preserve asynchronous payload lifetime. Disarm forces neutral immediately as the wanted value, without waiting on an in-flight pulse. Console work and a new arm are deferred while neutral is pending. A host RMT stub verifies initialization sends nothing, busy rejection, independent channel acceptance, latest-neutral replacement, unchanged in-flight buffers, and subsequent scheduled output. This stub does not measure physical pulses.
2. Kept the previously committed zero-spin neutral and translation cap fix in 7b1a885; the regression tests remain in the full suite.
3. Added mirrored(value, flipped), applied to heading trim and drive phase. The rotation test integrates force over 3600 raw headings for four stick directions and both orientations with a 0.6 rad body-frame lag, reflects the upside-down model as seen from above, and requires direction error below 10 degrees. Added trim sign checks.
4. Preserved the existing guide changes: full-charge 12.6/R estimate, continuous 2 A cap, ambiguous per-channel rating, meter verification, and worked example.
5. Mount LittleFS with format-on-fail at startup; load no run data at boot. On a nonempty armed-to-disarmed transition, retain RAM until both neutral channels have accepted, then write /run.bin in chunks of up to 32 entries, feeding the watchdog between writes. Failed writes remove the partial file and report storage state; failed mount does not abort control. Console log uses RAM first, otherwise streams saved entries with identical CSV formatting. Status exposes storage errors. Guide and spec describe persistence, replacement on each nonempty disarm, and allowing the write to finish before removing BEC power for USB handoff. Default filesystem partition confirmed below.
6. Added the host good-samples-to-NAN sequence above minRpm: translation starts unequal, sensor remains valid at 499 ms and becomes stale at 500 ms, then pulses are equal and nonzero with the LED off. Existing sensor/drive behavior already satisfies this regression. Guide and spec numbered failsafe step 6 now check live-to-stale while disarmed, startup refusal after unplugged reboot, and LED/disarm while armed. Blocks cannot demonstrate translation cutoff.
7. Status now separates startup=ok|failed from sensor=live|stale using tracker.sensorOk(nowMs). Updated guide references.
8. Preserved directional endpoint checks and the i6 Reverse instruction.
9. Kept the previously committed frame-after-100-ms-gap regression in 7b1a885; it passes in the full suite.
Guide addition: preserved the measured spin-up warning about the stall cap holding RPM below minRpm.

Installed API and partition inspection:

- C:/Users/austin/.platformio/packages/framework-arduinoespressif32/cores/esp32/esp32-hal-rmt.h and .c: rmtInit(pin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 1000000), rmtSetEOT(pin, LOW), and rmtWriteAsync(pin, data, 1). _rmtWrite returns false when TX_DONE is absent; asynchronous calls use blocking=false and loop=0. Payload pointers are passed to rmt_transmit, so buffers must remain stable.
- Installed libraries/LittleFS/src/LittleFS.h: begin(true) uses the partition label spiffs by default. FS.h provides binary File read/write and size methods used here.
- Installed framework-arduinoespressif32-esp32-libs/include/esp_system/include/esp_task_wdt.h confirms watchdog status, init, reconfigure, add and reset signatures. Existing 250 ms panic configuration is retained.
- Installed tools/partitions/default.csv includes `spiffs, data, spiffs, 0x290000, 0x160000`. Generated partitions.bin was uploaded. No custom partition configuration is needed.

Commands and observed output:

- Focused red command: `wsl -- bash -lc "cd /mnt/c/users/austin/onedrive/documents/github/Melty-ESP32 && PLATFORMIO_BUILD_DIR=/tmp/melty-pio ~/.venvs/pio/bin/pio test -e native -f test_drive"`. Initially the helper was absent (compile error); with the old identity behavior as the minimal helper stub, the force model failed `Expected TRUE Was FALSE` and the trim check failed `Expected -0.3 Was 0.3`. The failing process also reported SIGINT. After implementing mirroring, the exact full command below completed cleanly, 58/58, with no SIGINT.
- Full green command: `wsl -- bash -lc "cd /mnt/c/users/austin/onedrive/documents/github/Melty-ESP32 && PLATFORMIO_BUILD_DIR=/tmp/melty-pio ~/.venvs/pio/bin/pio test -e native"`. Final result including the additional RMT stub check is recorded below.
- `$env:PLATFORMIO_BUILD_DIR = 'C:/tmp/melty-pio'; python -m platformio run -e esp32`: SUCCESS, espressif32 8.0.0, Arduino 3.3.12. RAM 72756/327680 bytes, flash 431505/1310720 bytes.
- `$env:PLATFORMIO_BUILD_DIR = 'C:/tmp/melty-pio'; python -m platformio run -e esp32 -t upload`: SUCCESS, COM6 ESP32-D0WDQ6-V3, all written hashes verified, hard reset via RTS.
- `python C:/tmp/melty-task7-serial.py`: SUCCESS, using Python 3.14.0 and pyserial at 115200 baud. Script intentionally resets via RTS, captures status, toggles status off, reads original settings, sets radiusM=0.02 and saves, resets, observes 18 seconds without another reset, confirms persisted setting via show, runs log, restores radiusM=0.01 and saves, resets and confirms restoration. Full timestamped output follows.
- `git diff --check`: passed. `rg -n '\x{2014}' docs .superpowers/sdd/2026-10-30-melty-esp32-plans/task-7-report.md`: no matches. No LEDC code remains in src or lib.

Limits: COM6 has nothing wired. The armed path, physical RMT pulses, ESC signal-loss stopping, sensor/iBUS hardware behavior, and LittleFS run-write cannot be exercised without wiring and were not tested. No claim of operational robot safety is made. LittleFS mount and empty saved-log read were verified on the board. A completed saved run survives reset; removing power during a write can lose that run. Allow the disarm write to finish before disconnecting power. Defaults remain uncalibrated starting values.

Serial output:

```text
0.0s INTENTIONAL RESET
0.1s ets Jul 29 2019 12:21:46
0.1s rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
0.1s configsip: 0, SPIWP:0xee
0.1s clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
0.1s mode:DIO, clock div:2
0.1s load:0x3fff0030,len:4640
0.1s load:0x40078000,len:15632
0.1s load:0x40080400,len:3164
0.1s entry 0x4008059c
0.3s Melty ESP32 boot
0.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
0.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
0.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
1.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
1.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
1.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
1.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
1.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
2.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
2.1s COMMAND: status
2.8s COMMAND: show
2.8s radiusM=0.01
2.8s trimRadPerSec=3.14
2.8s rampPerSec=0.25
2.8s minRpm=200
2.8s stallCap=0.3
2.8s strength=0.25
2.8s phaseRad=0
2.8s ledArcRad=0.5
2.8s flipFilterMs=1000
2.8s outputHz=50
2.8s radialAxis=0
2.8s radialSign=1
2.8s zSign=1
2.8s reverseA=0
2.8s reverseB=0
3.4s COMMAND: set radiusM 0.02
4.1s COMMAND: save
4.1s Saved
4.8s INTENTIONAL RESET
4.9s ets Jul 29 2019 12:21:46
4.9s rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
4.9s configsip: 0, SPIWP:0xee
4.9s clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
4.9s mode:DIO, clock div:2
4.9s load:0x3fff0030,len:4640
4.9s load:0x40078000,len:15632
4.9s load:0x40080400,len:3164
4.9s entry 0x4008059c
5.1s Melty ESP32 boot
5.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
5.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
5.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
5.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
6.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
6.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
6.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
6.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
6.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
7.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
7.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
7.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
7.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
7.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
8.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
8.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
8.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
8.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
8.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
9.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
9.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
9.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
9.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
9.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
10.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
10.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
10.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
10.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
10.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
11.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
11.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
11.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
11.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
11.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
12.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
12.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
12.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
12.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
12.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
13.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
13.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
13.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
13.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
13.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
14.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
14.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
14.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
14.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
14.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
15.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
15.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
15.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
15.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
15.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
16.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
16.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
16.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
16.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
16.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
17.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
17.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
17.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
17.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
17.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
18.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
18.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
18.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
18.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
18.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
19.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
19.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
19.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
19.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
19.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
20.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
20.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
20.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
20.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
20.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
21.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
21.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
21.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
21.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
21.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
22.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
22.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
22.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
22.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
22.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
22.9s COMMAND: status
23.5s COMMAND: show
23.5s radiusM=0.02
23.5s trimRadPerSec=3.14
23.5s rampPerSec=0.25
23.5s minRpm=200
23.5s stallCap=0.3
23.5s strength=0.25
23.5s phaseRad=0
23.5s ledArcRad=0.5
23.5s flipFilterMs=1000
23.5s outputHz=50
23.5s radialAxis=0
23.5s radialSign=1
23.5s zSign=1
23.5s reverseA=0
23.5s reverseB=0
24.2s COMMAND: log
24.2s ms,rpm,radialG,aUs,bUs
24.2s No saved run (storage=ok)
24.8s COMMAND: set radiusM 0.01
25.5s COMMAND: save
25.5s Saved
26.2s INTENTIONAL RESET
26.3s ets Jul 29 2019 12:21:46
26.3s rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
26.3s configsip: 0, SPIWP:0xee
26.3s clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
26.3s mode:DIO, clock div:2
26.3s load:0x3fff0030,len:4640
26.3s load:0x40078000,len:15632
26.3s load:0x40080400,len:3164
26.3s entry 0x4008059c
26.5s Melty ESP32 boot
26.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
26.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
27.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
27.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
27.5s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
27.7s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
27.9s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
28.1s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
28.3s CH1=0 CH2=0 CH3=0 CH4=0 CH5=0 signal=lost startup=failed sensor=stale storage=ok radial=nan rpm=0 flipped=0
28.3s COMMAND: status
28.9s COMMAND: show
28.9s radiusM=0.01
28.9s trimRadPerSec=3.14
28.9s rampPerSec=0.25
28.9s minRpm=200
28.9s stallCap=0.3
28.9s strength=0.25
28.9s phaseRad=0
29.0s ledArcRad=0.5
29.0s flipFilterMs=1000
29.0s outputHz=50
29.0s radialAxis=0
29.0s radialSign=1
29.0s zSign=1
29.0s reverseA=0
29.0s reverseB=0
29.6s PASS: clean boot, 18 seconds without reset, set/save persisted, no saved run, original radius restored
```

Final full host output (exact full command above, clean exit 0):

```text
native test_console PASSED 00:00:16.690
native test_drive   PASSED 00:00:15.108
native test_esc     PASSED 00:00:15.606
native test_ibus    PASSED 00:00:11.739
native test_safety  PASSED 00:00:16.719
native test_smoke   PASSED 00:00:09.791
native test_spin    PASSED 00:00:12.733
59 test cases: 59 succeeded in 00:01:38.386
```

Final diff and documentation checks passed. The added native-only Arduino stub include path is excluded from the ESP32 environment. No production firmware changed after the successful flash and serial capture.
