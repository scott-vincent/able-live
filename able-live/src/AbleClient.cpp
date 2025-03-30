#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#include <math.h>
#endif
#include <iostream>
#include <allegro5/allegro_image.h>
#include "able_live.h"
#include "ChartFile.h"

// Externals
extern char* _pilotAwareData;
extern char* _fr24Data;
extern char* _gniusData;
extern char* _mobileData;
extern Settings _settings;
extern AircraftDrawData _aircraft;
extern bool _haveAble;
extern Locn _minLoc;
extern Locn _maxLoc;
extern PosData _aircraftData[MaxAircraft];
extern int _aircraftCount;
extern char _prevAbleData[36];
extern Trail _ableTrail[MaxAble];

// Variables
Locn minAbleLoc;
Locn maxAbleLoc;
bool haveAbleNum[MaxAble];
int ableAlt[MaxAble];

PosData me;
Locn meLoc[256];
int meCount = 0;
int mePos = -1;
double meIncrement;

// Prototypes
void cleanupBitmap(ALLEGRO_BITMAP* bmp);


char* jsonStr(char* str)
{
    static char value[16];

    value[0] = '\0';

    if (!str) {
        return value;
    }

    char* pos = strchr(str, ':');
    if (!pos) {
        return value;
    }

    pos = strchr(pos, '\"');
    if (!pos) {
        return value;
    }

    pos++;
    strncpy(value, pos, 15);
    value[15] = '\0';

    pos = strchr(value, '\"');
    if (pos) {
        *pos = '\0';
    }

    return value;
}

double jsonNum(char* str)
{
    static double value = 0;

    if (!str) {
        return value;
    }

    char* pos = strchr(str, ':');
    if (!pos) {
        return value;
    }

    sscanf(pos+1, "%lf", &value);
    return value;
}

void fixCallsign(char* callsign)
{
    if (strlen(callsign) < 4 || callsign[0] != 'G') {
        return;
    }

    char suffix[16];
    if (callsign[1] == '-') {
        strcpy(suffix, &callsign[2]);
    }
    else {
        strcpy(suffix, &callsign[1]);
    }

    if (strcmp(suffix, "UCAN") == 0) {
        strcpy(callsign, "ABLE01");
    }
    else if (strcmp(suffix, "OMJA") == 0) {
        strcpy(callsign, "ABLE02");
    }
    else if (strcmp(suffix, "BCIR") == 0) {
        strcpy(callsign, "ABLE03");
    }
    else if (strcmp(suffix, "BMFP") == 0) {
        strcpy(callsign, "ABLE04");
    }
    else if (strcmp(suffix, "IDOO") == 0) {
        strcpy(callsign, "ABLE05");
    }
    else if (strcmp(suffix, "SIXE") == 0) {
        strcpy(callsign, "ABLE06");
    }
    else if (strcmp(suffix, "IDID") == 0) {
        strcpy(callsign, "ABLE07");
    }
    else if (strcmp(suffix, "SAUP") == 0) {
        strcpy(callsign, "ABLE08");
    }
    else if (strcmp(suffix, "ICAN") == 0) {
        strcpy(callsign, "ABLE10");
    }
    else if (strcmp(suffix, "BNNY") == 0) {
        strcpy(callsign, "ABLE11");
    }
    else if (strcmp(suffix, "BRUD") == 0) {
        strcpy(callsign, "ABLE12");
    }
    else if (strcmp(suffix, "CDMA") == 0) {
        strcpy(callsign, "ABLE14");
    }
}

ALLEGRO_BITMAP* getBmp(char* callsign, char* typeCode)
{
    if (strncmp(callsign, "ABLE", 4) == 0) {
        return _aircraft.Able;
    }

    if (strcmp(typeCode, "GLID") == 0 || strcmp(typeCode, "DISC") == 0) {
        return _aircraft.Glider;
    }

    if (strcmp(typeCode, "GRND") == 0) {
        return _aircraft.Vehicle;
    }

    char prefixShort[8];
    char prefixModel[8];
    char prefixCallsign[8];
    sprintf(prefixShort, "_%.3s_", typeCode);
    sprintf(prefixModel, "_%.4s_", typeCode);
    sprintf(prefixCallsign, "_%.5s_", callsign);

    if (strstr(Helis, prefixShort) != NULL) {
        return _aircraft.Heli;
    }

    if (strstr(Turboprops, prefixShort) != NULL || strstr(Turboprops, prefixModel) != NULL) {
        return _aircraft.Turboprop;
    }

    if (strstr(Airliners, prefixShort) != NULL) {
        return _aircraft.Airliner;
    }

    if (strstr(Heavys, prefixShort) != NULL) {
        return _aircraft.Heavy;
    }

    if (strstr(Jets, prefixShort) != NULL) {
        return _aircraft.Jet;
    }

    if (strstr(Military_Helis, prefixShort) != NULL || strstr(Military_Helis, prefixCallsign) != NULL) {
        return _aircraft.Military_Heli;
    }

    if (strstr(Military_Jets, prefixShort) != NULL || strstr(Military_Jets, prefixCallsign) != NULL) {
        return _aircraft.Military_Jet;
    }

    if (strstr(Military_Smalls, prefixShort) != NULL || strstr(Military_Smalls, prefixCallsign) != NULL) {
        return _aircraft.Military_Small;
    }

    if (strstr(Military_Turboprops, prefixShort) != NULL || strstr(Military_Turboprop2s, prefixModel) != NULL) {
        return _aircraft.Military_Turboprop;
    }

    return _aircraft.Small;
}

void putMe(double lat, double lon)
{
    meLoc[meCount].lat = lat;
    meLoc[meCount].lon = lon;

    meCount++;
}

void circuitMe()
{
    putMe(51.32584, -0.83854);
    putMe(51.31723, -0.88315);
    putMe(51.30144, -0.87463);
    putMe(51.31236, -0.79748);
    putMe(51.32819, -0.81567);
    putMe(51.32580, -0.83847);
    putMe(51.324, -0.84778);
}

void localMe()
{
    putMe(51.32584, -0.83854);
    putMe(51.31723, -0.88315);
    putMe(51.401, -1.32);
    putMe(51.501, -1.82);
}

void initMe()
{
    meCount = 0;

    circuitMe();

    mePos = 0;
    me.loc.lat = meLoc[mePos].lat;
    me.loc.lon = meLoc[mePos].lon;
    me.heading = 0;

    meIncrement = 0.01;
}

bool addMe()
{
    if (mePos == meCount) {
        return false;
    }

    me.id = 12345;
    strcpy(me.callsign, "ABLE14");
    me.bmp = _aircraft.Able;
    me.altitude = 123;
    me.speed = 123;
    me.label.bmp = NULL;

    if (mePos == -1) {
        initMe();
        return (mePos < meCount);
    }

    if (me.loc.lat == meLoc[mePos].lat && me.loc.lon == meLoc[mePos].lon) {
        mePos++;
        if (mePos == meCount) {
            initMe();
            return (mePos < meCount);
        }
    }

    double latDiff = meLoc[mePos].lat - me.loc.lat;
    double lonDiff = meLoc[mePos].lon - me.loc.lon;

    double angle = atan(latDiff / lonDiff);
    double distance = sqrt(pow(latDiff, 2) + pow(lonDiff, 2));

    if (lonDiff < 0) {
        angle += ALLEGRO_PI;
    }

    me.heading = round(90 - angle * RadiansToDegrees);

    if (distance > meIncrement) {
        me.loc.lat = me.loc.lat + meIncrement * sin(angle);
        me.loc.lon = me.loc.lon + meIncrement * cos(angle);
    }
    else {
        me.loc.lat = meLoc[mePos].lat;
        me.loc.lon = meLoc[mePos].lon;
    }

    return true;
}

bool addAircraftData(char* pos, PosData* posData)
{
    int num = atoi(pos);
    if (num < 1 || num > 20) {
        printf("addAircraftData: Bad data (num out of range): %s\n", pos);
        return false;
    }

    pos = strchr(pos, ',');
    if (!pos) {
        printf("addAircraftData: Bad data (no typeCode): %s\n", pos);
        return false;
    }
    pos++;

    char* startPos = pos;
    pos = strchr(pos, ',');
    if (!pos) {
        printf("addAircraftData: Bad data (no lat): %s\n", pos);
        return false;
    }
    int len = pos - startPos;

    strncpy(posData->typeCode, startPos, len);
    posData->typeCode[len] = '\0';
    pos++;

    int count = sscanf(pos, "%lf,%lf,%d,%d,%d", &posData->loc.lat, &posData->loc.lon, &posData->heading, &posData->altitude, &posData->speed);
    if (count != 5) {
        printf("addAircraftData: Bad data (5->%d) -> %s\n", count, pos);
        return false;
    }

    if (haveAbleNum[num - 1]) {
        // Already have that Able
        //printf("addAircraftData: Already have ABLE%02d\n", num);
        return false;
    }

    if (posData->loc.lat > 52.22) {
        //printf("addAircraftData: Excluding ABLE%02d (not in Southern England)\n", num);
        return false;
    }

    posData->id = -1;

    if (num == 17) {
        strcpy(posData->callsign, "G-NIUS");
        posData->bmp = _aircraft.Gnius;
    }
    else if (num == 18) {
        strcpy(posData->callsign, "G-NIAI");
        posData->bmp = _aircraft.Gnius;
    }
    else if (num == 19) {
        strcpy(posData->callsign, "G-MOBL");
        posData->bmp = _aircraft.Gnius;
    }
    else if (num == 20) {
        strcpy(posData->callsign, "G-MOAI");
        posData->bmp = _aircraft.Gnius;
    }
    else if (num == 21) {
        strcpy(posData->callsign, "G-HANG");
        posData->bmp = _aircraft.Gnius;
    }
    else if (num == 22) {
        strcpy(posData->callsign, "G-HANI");
        posData->bmp = _aircraft.Gnius;
    }
    else {
        sprintf(posData->callsign, "ABLE%02d", num);
        posData->bmp = _aircraft.Able;
    }
    posData->label.bmp = NULL;

    //printf("addAircraftData: Added %s (%s) at %f,%f hdg: %d speed: %d alt: %d\n", posData->callsign, posData->typeCode, posData->loc.lat, posData->loc.lon, posData->heading, posData->speed, posData->altitude);
    return true;
}

void initArea()
{
    // Centre on Blackbushe
    _minLoc.lat = BlackbusheLat;
    _minLoc.lon = BlackbusheLon;
    _maxLoc.lat = BlackbusheLat;
    _maxLoc.lon = BlackbusheLon;

    // Blackbushe circuits
    minAbleLoc.lat = 51.298;
    minAbleLoc.lon = -0.9;
    maxAbleLoc.lat = 51.334;
    maxAbleLoc.lon = -0.79;

    if (_settings.keepBlackbusheZoomed) {
        _minLoc.lat = minAbleLoc.lat;
        _minLoc.lon = minAbleLoc.lon;
        _maxLoc.lat = maxAbleLoc.lat;
        _maxLoc.lon = maxAbleLoc.lon;
    }
}

void updateArea()
{
    bool isGnius = strncmp(_aircraftData[_aircraftCount].callsign, "G-NIUS", 6) == 0;
    bool isGniai = strncmp(_aircraftData[_aircraftCount].callsign, "G-NIAI", 6) == 0;
    bool isMobl = strncmp(_aircraftData[_aircraftCount].callsign, "G-MOBL", 6) == 0;
    bool isMoai = strncmp(_aircraftData[_aircraftCount].callsign, "G-MOAI", 6) == 0;

    if (strncmp(_aircraftData[_aircraftCount].callsign, "ABLE", 4) == 0 || isGnius || isGniai || isMobl || isMoai) {
        _aircraftData[_aircraftCount].isAble = true;
        _haveAble = true;

        int num;
        if (isGnius) {
            num = 17;
        }
        else if (isGniai) {
            num = 18;
        }
        else if (isMobl) {
            num = 19;
        }
        else if (isMoai) {
            num = 20;
        }
        else {
            num = atoi(&_aircraftData[_aircraftCount].callsign[4]);
        }

        if (num > 0) {
            num--;
        }

        haveAbleNum[num] = true;
        ableAlt[num] = _aircraftData[_aircraftCount].altitude;

        int i = _ableTrail[num].count;
        if (i < 6000) {
            _ableTrail[num].loc[i].lat = _aircraftData[_aircraftCount].loc.lat;
            _ableTrail[num].loc[i].lon = _aircraftData[_aircraftCount].loc.lon;
            time(&_ableTrail[num].lastUpdate);
            _ableTrail[num].count++;
        }
    }
    else {
        _aircraftData[_aircraftCount].isAble = false;
    }

    if (_aircraftData[_aircraftCount].isAble) {
        if (minAbleLoc.lat > _aircraftData[_aircraftCount].loc.lat) {
            minAbleLoc.lat = _aircraftData[_aircraftCount].loc.lat;
        }
        if (minAbleLoc.lon > _aircraftData[_aircraftCount].loc.lon) {
            minAbleLoc.lon = _aircraftData[_aircraftCount].loc.lon;
        }
        if (maxAbleLoc.lat < _aircraftData[_aircraftCount].loc.lat) {
            maxAbleLoc.lat = _aircraftData[_aircraftCount].loc.lat;
        }
        if (maxAbleLoc.lon < _aircraftData[_aircraftCount].loc.lon) {
            maxAbleLoc.lon = _aircraftData[_aircraftCount].loc.lon;
        }
    }
    else {
        if (_minLoc.lat > _aircraftData[_aircraftCount].loc.lat) {
            _minLoc.lat = _aircraftData[_aircraftCount].loc.lat;
        }
        if (_minLoc.lon > _aircraftData[_aircraftCount].loc.lon) {
            _minLoc.lon = _aircraftData[_aircraftCount].loc.lon;
        }
        if (_maxLoc.lat < _aircraftData[_aircraftCount].loc.lat) {
            _maxLoc.lat = _aircraftData[_aircraftCount].loc.lat;
        }
        if (_maxLoc.lon < _aircraftData[_aircraftCount].loc.lon) {
            _maxLoc.lon = _aircraftData[_aircraftCount].loc.lon;
        }
    }
}

void writeAbleData()
{
    char ableData[36];
    strcpy(ableData, "--,--,--,--,--,--,--,--,--,--,--,--");

    if (_haveAble) {
        for (int i = 0; i < 12; i++) {
            if (haveAbleNum[i]) {
                int alt = 0;
                if (ableAlt[i] > 99999) {
                    alt = 99;
                }
                else if (ableAlt[i] > 0) {
                    alt = round(ableAlt[i] / 100);
                }
                ableData[i * 3] = '0' + (alt / 10);
                ableData[i * 3 + 1] = '0' + (alt % 10);
            }
        }
    }

    if (strcmp(_prevAbleData, ableData) != 0) {
        strcpy(_prevAbleData, ableData);
        FILE* outf = fopen(WriteDataFile, "w");
        if (!outf) {
            printf("Failed to write file: %s\n", WriteDataFile);
            return;
        }
        fprintf(outf, "%s\n", ableData);
        fclose(outf);
    }
}

bool GetLiveData()
{
    char* pos = strstr(_pilotAwareData, "\"acList\":");
    if (!pos) {
        //printf("Unexpected response: %s\n", _pilotAwareData);
        _minLoc.lat = BlackbusheLat - 0.05;
        _minLoc.lon = BlackbusheLon - 0.16;
        _maxLoc.lat = BlackbusheLat + 0.13;
        _maxLoc.lon = BlackbusheLon + 0.37;
        return false;
    }

    // Cleanup old labels
    for (int i = 0; i < _aircraftCount; i++) {
        if (_aircraftData[i].label.bmp) {
            cleanupBitmap(_aircraftData[i].label.bmp);
        }
    }

    _aircraftCount = 0;
    _haveAble = false;

    for (int i = 0; i < MaxAble; i++) {
        haveAbleNum[i] = false;
    }

    initArea();

    time_t now;
    time(&now);

    while (pos) {
        pos = strstr(pos, "\"Id\":");
        if (!pos) {
            break;
        }
        _aircraftData[_aircraftCount].id = jsonNum(pos);

        pos = strstr(pos, "\"Alt\":");
        if (!pos) {
            break;
        }
        _aircraftData[_aircraftCount].altitude = jsonNum(pos);
        if (_aircraftData[_aircraftCount].altitude < 100) {
            _aircraftData[_aircraftCount].altitude = 0;
        }

        pos = strstr(pos, "\"Call\":");
        if (!pos) {
            break;
        }
        strcpy(_aircraftData[_aircraftCount].callsign, jsonStr(pos));

        pos = strstr(pos, "\"Lat\":");
        if (!pos) {
            break;
        }
        _aircraftData[_aircraftCount].loc.lat = jsonNum(pos);;

        pos = strstr(pos, "\"Long\":");
        if (!pos) {
            break;
        }
        _aircraftData[_aircraftCount].loc.lon = jsonNum(pos);;

        pos = strstr(pos, "\"PosTime\":");
        if (!pos) {
            break;
        }
        time_t posTime = jsonNum(pos) / 1000;

        // Ignore stale aircraft
        if (now - posTime > 30) {
            continue;
        }

        pos = strstr(pos, "\"Spd\":");
        if (!pos) {
            break;
        }
        _aircraftData[_aircraftCount].speed = jsonNum(pos);;

        pos = strstr(pos, "\"Trak\":");
        if (!pos) {
            break;
        }
        _aircraftData[_aircraftCount].heading = jsonNum(pos);;

        pos = strstr(pos, "\"Type\":");
        if (!pos) {
            break;
        }
        strncpy(_aircraftData[_aircraftCount].typeCode, jsonStr(pos), 7);
        _aircraftData[_aircraftCount].typeCode[7] = '\0';

        fixCallsign(_aircraftData[_aircraftCount].callsign);
        _aircraftData[_aircraftCount].bmp = getBmp(_aircraftData[_aircraftCount].callsign, _aircraftData[_aircraftCount].typeCode);
        _aircraftData[_aircraftCount].label.bmp = NULL;

        updateArea();
        _aircraftCount++;
    }

#ifdef DEBUG
    if (addMe()) {
        memcpy(&_aircraftData[_aircraftCount], &me, sizeof(PosData));
        updateArea();
        _aircraftCount++;
    }
#endif

    // Add in FlightRadar data but only if Able num is completely missing
    pos = strchr(_fr24Data, '#');
    while (pos) {
        pos++;
        if (addAircraftData(pos, &_aircraftData[_aircraftCount])) {
            updateArea();
            _aircraftCount++;
        }
        pos = strchr(pos, '#');
    }

    // Add in G-NIUS data
    pos = strchr(_gniusData, '#');
    while (pos) {
        pos++;
        if (addAircraftData(pos, &_aircraftData[_aircraftCount])) {
            updateArea();
            _aircraftCount++;
        }
        pos = strchr(pos, '#');
    }

    // Add in Mobile data
    pos = strchr(_mobileData, '#');
    while (pos) {
        pos++;
        if (addAircraftData(pos, &_aircraftData[_aircraftCount])) {
            updateArea();
            _aircraftCount++;
        }
        pos = strchr(pos, '#');
    }

    writeAbleData();

    // ABLE aircraft stale after 30 seconds
    for (int i = 0; i < 14; i++) {
        if (!haveAbleNum[i] && _ableTrail[i].count > 0 && now - _ableTrail[i].lastUpdate > 30) {
            _ableTrail[i].count = 0;
        }
    }

    // G-NIUS aircraft stale after 3 seconds
    for (int i = 16; i < MaxAble; i++) {
        if (!haveAbleNum[i] && _ableTrail[i].count > 0 && now - _ableTrail[i].lastUpdate > 2) {
            _ableTrail[i].count = 0;
        }
    }

    if (_haveAble) {
        _minLoc.lat = minAbleLoc.lat;
        _minLoc.lon = minAbleLoc.lon;
        _maxLoc.lat = maxAbleLoc.lat;
        _maxLoc.lon = maxAbleLoc.lon;

        // Limit area to maximum large map size
        if (_minLoc.lat < MinLatLarge) {
            _minLoc.lat = MinLatLarge;
        }
        if (_minLoc.lon < MinLonLarge) {
            _minLoc.lon = MinLonLarge;
        }
        if (_maxLoc.lat > MaxLatLarge) {
            _maxLoc.lat = MaxLatLarge;
        }
        if (_maxLoc.lon > MaxLonLarge) {
            _maxLoc.lon = MaxLonLarge;
        }
    }
    else {
        // Limit area to Blackbushe, Heathrow and Farnborough
        if (_minLoc.lat < BlackbusheLat - 0.05) {
            _minLoc.lat = BlackbusheLat - 0.05;
        }
        if (_minLoc.lon < BlackbusheLon - 0.16) {
            _minLoc.lon = BlackbusheLon - 0.16;
        }
        if (_maxLoc.lat > BlackbusheLat + 0.13) {
            _maxLoc.lat = BlackbusheLat + 0.13;
        }
        if (_maxLoc.lon > BlackbusheLon + 0.37) {
            _maxLoc.lon = BlackbusheLon + 0.37;
        }
    }

    if (_settings.keepBlackbusheZoomed) {
        initArea();
    }

    return true;
}
