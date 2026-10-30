#pragma once
#include "settings.h"
enum class ConsoleResult { Ok, Save, Defaults, DumpLog, ToggleStatus, Rejected };
ConsoleResult handleLine(const char* line, bool armed, Settings& s);
