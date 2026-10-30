#include "store.h"
#include <Preferences.h>
#include <cstdlib>
struct StoredSettings { uint32_t version; Settings settings; };
void storeLoad(Settings& s) {
    s = Settings{}; Preferences p;
    if (!p.begin("melty", true)) return;
    StoredSettings blob{};
    if (p.getBytesLength("settings") == sizeof(blob) && p.getBytes("settings", &blob, sizeof(blob)) == sizeof(blob) && blob.version == 1 && settingsValid(blob.settings)) s = blob.settings;
    p.end();
}
void storeSave(const Settings& s) {
    Preferences p; if (!p.begin("melty", false)) abort();
    StoredSettings blob{1, s}; if (p.putBytes("settings", &blob, sizeof(blob)) != sizeof(blob)) abort(); p.end();
}

#include <LittleFS.h>
#include <esp_task_wdt.h>
static bool mounted;
static const char* runStatus = "unmounted";
void storeBegin() {
    mounted = LittleFS.begin(true);
    runStatus = mounted ? "ok" : "mount-failed";
}
const char* storeRunStatus() { return runStatus; }
void storeSaveRun(const RunLog<3000>& log) {
    if (!mounted || !log.size()) return;
    File file = LittleFS.open("/run.bin", FILE_WRITE);
    bool ok = static_cast<bool>(file);
    for (size_t i = 0; ok && i < log.size();) {
        LogEntry chunk[32];
        size_t count = 0;
        while (count < 32 && i < log.size()) chunk[count++] = log.at(i++);
        const size_t bytes = count * sizeof(LogEntry);
        ok = file.write(reinterpret_cast<const uint8_t*>(chunk), bytes) == bytes;
    }
    file.close();
    // File::close() discards flush errors, so confirm what actually reached flash.
    if (ok) {
        File saved = LittleFS.open("/run.bin", FILE_READ);
        ok = saved && saved.size() == log.size() * sizeof(LogEntry);
    }
    if (!ok) LittleFS.remove("/run.bin");
    runStatus = ok ? "ok" : "write-failed";
}
void storePrintEntry(Print& output, const LogEntry& e) {
    output.printf("%lu,%g,%g,%u,%u\n", static_cast<unsigned long>(e.ms), e.rpm, e.radialG, e.aUs, e.bUs);
    esp_task_wdt_reset();
}
void storeDumpRun(Print& output) {
    if (!mounted) { output.println("Run log unavailable: mount-failed"); return; }
    if (!LittleFS.exists("/run.bin")) {
        output.printf("No saved run (storage=%s)\n", runStatus); return;
    }
    File file = LittleFS.open("/run.bin", FILE_READ);
    if (!file || file.size() == 0 || file.size() % sizeof(LogEntry)) {
        runStatus = "read-failed"; output.println("Run log read failed"); return;
    }
    while (file.available()) {
        LogEntry entry;
        if (file.read(reinterpret_cast<uint8_t*>(&entry), sizeof(entry)) != sizeof(entry)) {
            runStatus = "read-failed"; output.println("Run log read failed"); break;
        }
        storePrintEntry(output, entry);
    }
}
