#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <cmath>
#include <cstring>
#include <console.h>
#include <ibus.h>
#include <safety.h>
#include <spin.h>
#include <runlog.h>
#include "esc.h"
#include "mpu.h"
#include "store.h"

static Settings settings;
static Safety safety;
static IbusParser parser;
static IbusFrame frame{};
static HeadingTracker tracker;
static FlipDetector flip;
static JamGuard jam;
static RunLog<3000> runLog;
static bool startupOk, statusEnabled = true, wasArmed;
static float radial, spin;
static uint32_t lastLoop, lastFlip, lastOutput, lastLog, lastStatus, lastControlMs;
static char line[80];
static size_t lineLength;
static bool overflow;
static constexpr uint32_t kMaxRxGapUs = 20000;
struct SensorSample { float xG, yG, zG; bool ok; uint32_t seq; };
static SensorSample sensorSample{};
static portMUX_TYPE sensorMux = portMUX_INITIALIZER_UNLOCKED;
static uint32_t lastSensorSeq, outputHz;
static float lastGoodZ;
static bool hasGoodZ;

static void sensorTask(void*) {
    for (;;) {
        SensorSample sample{};
        sample.ok = mpuRead(sample.xG, sample.yG, sample.zG);
        portENTER_CRITICAL(&sensorMux);
        sample.seq = sensorSample.seq + 1;
        sensorSample = sample;
        portEXIT_CRITICAL(&sensorMux);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void consolePoll() {
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());
        if (c == '\r') continue;
        if (c != '\n') { if (lineLength < 79 && !overflow) line[lineLength++] = c; else overflow = true; continue; }
        line[lineLength] = 0;
        uint32_t oldHz = settings.outputHz;
        ConsoleResult result = overflow ? ConsoleResult::Rejected : handleLine(line, false, settings);
        if (result == ConsoleResult::Defaults) settings = Settings{};
        if (settings.outputHz != oldHz) Serial.println("Save and reboot to apply outputHz");
        if (result == ConsoleResult::Save) { storeSave(settings); Serial.println("Saved"); }
        else if (result == ConsoleResult::Rejected) Serial.println("Rejected");
        else if (result == ConsoleResult::ToggleStatus) statusEnabled = !statusEnabled;
        else if (result == ConsoleResult::DumpLog) {
            Serial.println("ms,rpm,radialG,aUs,bUs");
            for (size_t i = 0; i < runLog.size(); ++i) {
                const LogEntry& e = runLog.at(i);
                Serial.printf("%lu,%g,%g,%u,%u\n", static_cast<unsigned long>(e.ms), e.rpm, e.radialG, e.aUs, e.bUs);
                esp_task_wdt_reset();
            }
        } else if (result == ConsoleResult::Ok && strcmp(line, "show") == 0) {
            char output[512]; formatSettings(settings, output, sizeof(output)); Serial.print(output);
        }
        lineLength = 0; overflow = false;
    }
}
void setup() {
    storeLoad(settings);
    outputHz = settings.outputHz;
    escBegin(outputHz);
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, -1);
    pinMode(27, OUTPUT); digitalWrite(27, LOW);
    startupOk = mpuBegin() && mpuRestCheck(); safety.setSensorStartupOk(startupOk);
    esp_log_level_set("*", ESP_LOG_NONE);
    if (xTaskCreatePinnedToCore(sensorTask, "sensor", 4096, nullptr, 1, nullptr, 0) != pdPASS) abort();
    esp_task_wdt_config_t config{250, 0, true};
    esp_err_t result = esp_task_wdt_status(nullptr);
    if (result == ESP_ERR_INVALID_STATE) result = esp_task_wdt_init(&config);
    else if (result == ESP_OK || result == ESP_ERR_NOT_FOUND) result = esp_task_wdt_reconfigure(&config);
    if (result != ESP_OK || esp_task_wdt_add(nullptr) != ESP_OK) abort();
    Serial.println("Melty ESP32 boot");
    lastLoop = micros(); lastControlMs = millis();
}
void loop() {
    uint32_t nowUs = micros();
    if (nowUs - lastLoop < 1000) return;
    uint32_t dtUs = nowUs - lastLoop; lastLoop = nowUs;
    if (dtUs > kMaxRxGapUs) {
        while (Serial2.available()) Serial2.read();
        parser = IbusParser{};
    }
    uint32_t nowMs = millis(), dtMs = nowMs - lastControlMs; lastControlMs = nowMs;
    if (wasArmed) {
        lineLength = 0; overflow = false;
        while (Serial.available()) Serial.read();
    }
    int available = Serial2.available();
    for (int i = 0; i < available; ++i) {
        IbusFrame incoming;
        if (parser.push(static_cast<uint8_t>(Serial2.read()), incoming)) {
            frame = incoming; safety.onFrame(frame, nowMs);
            if (wasArmed && !safety.armed()) { escWrite(kNeutral); digitalWrite(27, LOW); wasArmed = false; }
            else if (!wasArmed && safety.armed()) { runLog.clear(); spin = 0; jam = JamGuard{}; wasArmed = true; }
        }
    }
    safety.update(nowMs);
    bool armed = safety.armed();
    if (wasArmed && !armed) { escWrite(kNeutral); digitalWrite(27, LOW); }
    if (!wasArmed && armed) { runLog.clear(); spin = 0; jam = JamGuard{}; }
    wasArmed = armed;
    portENTER_CRITICAL(&sensorMux);
    SensorSample sample = sensorSample;
    portEXIT_CRITICAL(&sensorMux);
    bool readOk = sample.seq != lastSensorSeq && sample.ok;
    lastSensorSeq = sample.seq;
    if (readOk) { lastGoodZ = sample.zG; hasGoodZ = true; }
    radial = readOk ? (settings.radialAxis == 0 ? sample.xG : sample.yG) * settings.radialSign : NAN;
    tracker.update(radial, settings.radiusM, stick(frame.ch[3]) * settings.trimRadPerSec, dtUs, nowMs);
    if (nowMs - lastFlip >= 10) { lastFlip = nowMs; if (hasGoodZ) flip.update(lastGoodZ, settings.zSign, settings.flipFilterMs, 10); }
    if (!armed) {
        spin = 0; escWrite(kNeutral); digitalWrite(27, LOW);
        consolePoll();
        if (statusEnabled && nowMs - lastStatus >= 200) {
            lastStatus = nowMs;
            Serial.printf("CH1=%u CH2=%u CH3=%u CH4=%u CH5=%u signal=%s sensor=%s radial=%g rpm=%g flipped=%d\n", frame.ch[0], frame.ch[1], frame.ch[2], frame.ch[3], frame.ch[4], safety.signalOk() ? "ok" : "lost", startupOk ? "ok" : "failed", radial, tracker.rpm(), flip.flipped());
        }
        esp_task_wdt_reset(); return;
    }
    lineLength = 0; overflow = false;
    while (Serial.available()) Serial.read();
    spin = rampStep(spin, spinCommand(frame.ch[2]), settings.rampPerSec, dtMs);
    float power = jam.apply(spin, tracker.rpm(), settings.stallCap, settings.minRpm, dtMs);
    bool sensorOk = tracker.sensorOk(nowMs);
    float heading = effectiveHeading(tracker.headingRad(), flip.flipped());
    MotorPulses pulses = computePulses({power, stick(frame.ch[0]), stick(frame.ch[1]), heading, tracker.rpm(), sensorOk}, {settings.minRpm, settings.strength, settings.phaseRad, settings.reverseA, settings.reverseB});
    if (nowUs - lastOutput >= 1000000 / outputHz) { lastOutput = nowUs; escWrite(pulses); }
    digitalWrite(27, ledOn(heading, settings.ledArcRad, sensorOk) ? HIGH : LOW);
    if (nowMs - lastLog >= 20) { lastLog = nowMs; runLog.add({nowMs, tracker.rpm(), radial, pulses.aUs, pulses.bUs}); }
    esp_task_wdt_reset();
}
