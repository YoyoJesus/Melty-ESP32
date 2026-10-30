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
