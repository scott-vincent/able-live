#pragma once
#ifdef _WINDOWS
#include <windows.h>
#endif

void saveSettings();
void loadSettings();
bool loadCalibrationData(const char* filename, ChartData* chartData);
bool readUrl(const char* filename, char* url);
