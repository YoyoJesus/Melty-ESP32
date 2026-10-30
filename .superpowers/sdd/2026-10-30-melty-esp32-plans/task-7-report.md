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
