#ifdef _WINDOWS
#include <windows.h>
#endif
#include <iostream>
#include "able_live.h"
#include "ChartCoords.h"

// Constants
const char SettingsExt[] = ".settings";
const char CalibrationExt[] = ".calibration";
const char urlQuery[] = "https://www.openstreetmap.org/search?query=";

// Externals
extern int _displayWidth;
extern int _displayHeight;
extern char _currentChart[256];
extern Settings _settings;
extern char _moduleFilename[256];


/// <summary>
/// Returns the pathname of the settings file
/// </summary>
char* settingsFile()
{
    static char filename[256];
    strcpy(filename, _moduleFilename);

    char* last = strrchr(filename, '\\');
    if (!last) {
        last = strrchr(filename, '/');
    }

    if (last) {
        last++;
    }
    else {
        last = filename;
    }

    char* ext = strrchr(last, '.');
    if (ext) {
        *ext = '\0';
    }
    
    strcat(filename, SettingsExt);

    return last;
}

/// <summary>
/// Returns the pathname of the chart calibration file
/// </summary>
char* calibrationFile(const char *chartFile)
{
    static char filename[256];
    strcpy(filename, chartFile);

    char* ext = strrchr(filename, '.');
    if (ext) {
        *ext = '\0';
    }

    strcat(filename, CalibrationExt);

    return filename;
}

/// <summary>
/// Save the latest window position/size and chart file name
/// </summary>
void saveSettings()
{
    FILE* outf = fopen(settingsFile(), "w");
    if (!outf) {
        printf("Failed to write file %s", settingsFile());
        return;
    }

    fprintf(outf, "%d,%d,%d,%d\n", _settings.x, _settings.y, _settings.width, _settings.height);
    fprintf(outf, "%d,%d,%d,%d,%d\n", _settings.tags, _settings.addFlightradarData, _settings.southernEnglandOnly, _settings.excludeHighAlt, _settings.keepBlackbusheZoomed);

    fclose(outf);
}

/// <summary>
/// Load the last window position/size and chart file name if it exists.
/// </summary>
void loadSettings()
{
    FILE* inf = fopen(settingsFile(), "r");
    if (inf != NULL) {
        char line[256];
        int nextLine = 1;
        int x;
        int y;
        int width;
        int height;
        int val1, val2, val3, val4, val5;

        *_settings.location = '\0';

        while (fgets(line, 256, inf)) {
            while (strlen(line) > 0 && (line[strlen(line) - 1] == ' '
                || line[strlen(line) - 1] == '\r' || line[strlen(line) - 1] == '\n')) {
                line[strlen(line) - 1] = '\0';
            }

            if (strlen(line) == 0) {
                continue;
            }

            switch (nextLine) {
            case 1:
            {
                int items = sscanf(line, "%d,%d,%d,%d", &x, &y, &width, &height);
                if (items == 4) {
                    _settings.x = x;
                    _settings.y = y;
                    _settings.width = width;
                    _settings.height = height;
                }
                break;
            }
            case 2:
            {
                int items = sscanf(line, "%d,%d,%d,%d,%d", &val1, &val2, &val3, &val4, &val5);
                if (items == 5) {
                    _settings.tags = val1;
                    _settings.addFlightradarData = val2;
                    _settings.southernEnglandOnly = val3,
                    _settings.excludeHighAlt = val4;
                    _settings.keepBlackbusheZoomed = val5;
                }
                break;
            }
            }

            nextLine++;
        }

        fclose(inf);
    }

    _settings.framesPerSec = DefaultFPS;
}

bool loadCalibrationData(const char* chartFile, ChartData* chartData)
{
    char *filename = calibrationFile(chartFile);

    FILE* inf = fopen(filename, "r");
    if (!inf) {
        printf("Missing file: %s\n", filename);
        return false;
    }

    char line[256];
    int x;
    int y;
    double lat;
    double lon;

    int state = 0;
    while (fgets(line, 256, inf) && state < 2) {
        while (strlen(line) > 0 && (line[strlen(line) - 1] == ' '
            || line[strlen(line) - 1] == '\r' || line[strlen(line) - 1] == '\n')) {
            line[strlen(line) - 1] = '\0';
        }

        if (strlen(line) > 0) {
            int items = sscanf(line, "%d,%d = %lf,%lf", &x, &y, &lat, &lon);
            if (items == 4) {
                chartData->x[state] = x;
                chartData->y[state] = y;
                chartData->lat[state] = lat;
                chartData->lon[state] = lon;
                state++;
            }
        }
    }

    fclose(inf);
    return true;
}

bool readUrl(const char* filename, char* url)
{
    FILE* inf = fopen(filename, "r");
    if (!inf) {
        printf("URL file not found: %s\n", filename);
        return false;
    }

    int size = fread(url, 1, 256, inf);
    url[size] = '\0';

    char* pos = strchr(url, '\r');
    if (pos) {
        *pos = '\0';
    }

    pos = strchr(url, '\n');
    if (pos) {
        *pos = '\0';
    }

    fclose(inf);

    return true;
}

char* getGniusData()
{
    static char gniusData[256];

    static char command[256];
    strcpy(command, _moduleFilename);

    char* last = strrchr(command, '\\');
    if (!last) {
        last = strrchr(command, '/');
    }

    if (last) {
        last++;
    }
    else {
        last = command;
    }

#ifdef _WINDOWS
    strcpy(last, "gnius-sendevent.exe simvars");
#else
    strcpy(last, "gnius-sendevent simvars");
#endif

    FILE* pipe = _popen(command, "r");
    if (!pipe) {
        printf("Failed to run %s\n", command);
        *gniusData = '\0';
        return gniusData;
    }

    int bytes = fread(gniusData, 1, 256, pipe);
    gniusData[bytes] = '\0';
    fclose(pipe);

    return gniusData;
}