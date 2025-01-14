#pragma once
#ifdef _WINDOWS
#include <windows.h>
#endif
#include "able_live.h"

void saveSettings();
void loadSettings();
bool loadCalibrationData(const char* filename, ChartData* chartData);
bool readUrl(const char* filename, char* url);
void milliSleep(int milliSecs);
