#include "console.h"
#include <cstring>
#include <cctype>

ConsoleResult handleLine(const char* line, bool armed, Settings& s) {
    if (armed || !line) return ConsoleResult::Rejected;
    size_t len = 0;
    while (len <= 79 && line[len]) ++len;
    if (!len || len > 79) return ConsoleResult::Rejected;
    char buffer[80]; memcpy(buffer, line, len + 1);
    char* tokens[4]; size_t count = 0; char* p = buffer;
    while (*p) {
        while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
        if (!*p) break;
        if (count == 4) return ConsoleResult::Rejected;
        tokens[count++] = p;
        while (*p && !std::isspace(static_cast<unsigned char>(*p))) ++p;
        if (*p) *p++ = '\0';
    }
    if (count == 3 && !strcmp(tokens[0], "set")) return setByName(s, tokens[1], tokens[2]) ? ConsoleResult::Ok : ConsoleResult::Rejected;
    if (count != 1) return ConsoleResult::Rejected;
    if (!strcmp(tokens[0], "show")) return ConsoleResult::Ok;
    if (!strcmp(tokens[0], "save")) return ConsoleResult::Save;
    if (!strcmp(tokens[0], "defaults")) return ConsoleResult::Defaults;
    if (!strcmp(tokens[0], "log")) return ConsoleResult::DumpLog;
    if (!strcmp(tokens[0], "status")) return ConsoleResult::ToggleStatus;
    return ConsoleResult::Rejected;
}
