#pragma once
#include <settings.h>
void storeLoad(Settings& s);
void storeSave(const Settings& s);
#include <runlog.h>
#include <Print.h>
void storeBegin();
const char* storeRunStatus();
void storeSaveRun(const RunLog<3000>& log);
void storePrintEntry(Print& output, const LogEntry& entry);
void storeDumpRun(Print& output);
