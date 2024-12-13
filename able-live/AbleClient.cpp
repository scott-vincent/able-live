#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#include <math.h>
#endif
#include <iostream>
#include <allegro5/allegro_image.h>
#include "able_live.h"

// Externals
extern bool _serverQuit;
extern bool _fr24Request;
extern bool _ableRequest;
extern bool _ableResponse;
extern char* _ableData;
extern Settings _settings;
extern AircraftDrawData _aircraft;
extern bool _haveAble;
extern Locn _minLoc;
extern Locn _maxLoc;
extern PosData _aircraftData[2][MaxAircraft];
extern int _set;
extern int _aircraftCount;

// Variables
Locn minAbleLoc;
Locn maxAbleLoc;
bool haveAbleNum[16];

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
    if (callsign[0] != 'G' || callsign[1] != '-') {
        return;
    }

    if (callsign[2] == 'B') {
        if (strcmp(&callsign[3], "CIR") == 0) {
            strcpy(callsign, "ABLE03");
        }
        else if (strcmp(&callsign[3], "MFP") == 0) {
            strcpy(callsign, "ABLE04");
        }
        else if (strcmp(&callsign[3], "LVI") == 0) {
            strcpy(callsign, "ABLE08");
        }
        else if (strcmp(&callsign[3], "NNY") == 0) {
            strcpy(callsign, "ABLE11");
        }
        else if (strcmp(&callsign[3], "RUD") == 0) {
            strcpy(callsign, "ABLE12");
        }
    }
    else if (callsign[2] == 'I') {
        if (strcmp(&callsign[3], "DOO") == 0) {
            strcpy(callsign, "ABLE05");
        }
        else if (strcmp(&callsign[3], "DID") == 0) {
            strcpy(callsign, "ABLE07");
        }
        else if (strcmp(&callsign[3], "CAN") == 0) {
            strcpy(callsign, "ABLE10");
        }
    }
    else if (strcmp(&callsign[2], "UCAN") == 0) {
        strcpy(callsign, "ABLE01");
    }
    else if (strcmp(&callsign[2], "OMJA") == 0) {
        strcpy(callsign, "ABLE02");
    }
    else if (strcmp(&callsign[2], "SIXE") == 0) {
        strcpy(callsign, "ABLE06");
    }
    else if (strcmp(&callsign[2], "CDMA") == 0) {
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
    strcpy(me.callsign, "ABLE15");
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

bool addFr24(char *pos, PosData* posData)
{
    int num;

    int count = sscanf(pos, "%d,%7s,%lf,%lf,%d,%d,%d", &num, posData->typeCode, &posData->loc.lat, &posData->loc.lon, &posData->heading, &posData->altitude, &posData->speed);
    posData->typeCode[7] = '\0';

    if (count != 7) {
        printf("addFr24: Bad data -> %s\n", pos);
        return false;
    }

    if (haveAbleNum[num - 1]) {
        // Already have that Able
        //printf("addFr24: Already have ABLE%02d\n", num);
        return false;
    }

    if (posData->loc.lat > 52.22) {
        //printf("addFr24: Excluding ABLE%02d (not in Southern England)\n", num);
        return false;
    }

    posData->id = -1;
    sprintf(posData->callsign, "ABLE%02d", num);
    posData->bmp = _aircraft.Able;
    posData->label.bmp = NULL;

    printf("addFr24: Added %s (%s) at %f,%f hdg: %d speed: %d alt: %d\n", posData->callsign, posData->typeCode, posData->loc.lat, posData->loc.lon, posData->heading, posData->speed, posData->altitude);
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
    if (strncmp(_aircraftData[_set][_aircraftCount].callsign, "ABLE", 4) == 0) {
        _aircraftData[_set][_aircraftCount].isAble = true;
        _haveAble = true;

        int num = atoi(&_aircraftData[_set][_aircraftCount].callsign[4]);
        haveAbleNum[num - 1] = true;
    }
    else {
        _aircraftData[_set][_aircraftCount].isAble = false;
    }

    if (_aircraftData[_set][_aircraftCount].isAble) {
        if (minAbleLoc.lat > _aircraftData[_set][_aircraftCount].loc.lat) {
            minAbleLoc.lat = _aircraftData[_set][_aircraftCount].loc.lat;
        }
        if (minAbleLoc.lon > _aircraftData[_set][_aircraftCount].loc.lon) {
            minAbleLoc.lon = _aircraftData[_set][_aircraftCount].loc.lon;
        }
        if (maxAbleLoc.lat < _aircraftData[_set][_aircraftCount].loc.lat) {
            maxAbleLoc.lat = _aircraftData[_set][_aircraftCount].loc.lat;
        }
        if (maxAbleLoc.lon < _aircraftData[_set][_aircraftCount].loc.lon) {
            maxAbleLoc.lon = _aircraftData[_set][_aircraftCount].loc.lon;
        }
    }
    else {
        if (_minLoc.lat > _aircraftData[_set][_aircraftCount].loc.lat) {
            _minLoc.lat = _aircraftData[_set][_aircraftCount].loc.lat;
        }
        if (_minLoc.lon > _aircraftData[_set][_aircraftCount].loc.lon) {
            _minLoc.lon = _aircraftData[_set][_aircraftCount].loc.lon;
        }
        if (_maxLoc.lat < _aircraftData[_set][_aircraftCount].loc.lat) {
            _maxLoc.lat = _aircraftData[_set][_aircraftCount].loc.lat;
        }
        if (_maxLoc.lon < _aircraftData[_set][_aircraftCount].loc.lon) {
            _maxLoc.lon = _aircraftData[_set][_aircraftCount].loc.lon;
        }
    }
}

bool GetLiveData()
{
    if (_ableRequest || _ableResponse)
        return true;

    char fr24Data[1024];
    *fr24Data = '\0';

    if (_settings.addFlightradarData) {
        _fr24Request = true;
        _ableRequest = true;

        while (!_ableResponse) {
            al_rest(0.02);
            if (_serverQuit) {
                return false;
            }
        }
        _ableResponse = false;
        strcpy(fr24Data, _ableData);
    }

    _fr24Request = false;
    _ableRequest = true;

    while (!_ableResponse) {
        al_rest(0.02);
        if (_serverQuit) {
            return false;
        }
    }
    _ableResponse = false;

    char* pos = strstr(_ableData, "\"acList\":");
    if (!pos) {
        printf("Unexpected response: %s\n", _ableData);
        _minLoc.lat = BlackbusheLat - 0.05;
        _minLoc.lon = BlackbusheLon - 0.16;
        _maxLoc.lat = BlackbusheLat + 0.13;
        _maxLoc.lon = BlackbusheLon + 0.37;
        return false;
    }

    int oldSet = _set;
    int oldCount = _aircraftCount;

    _set = 1 - _set;
    _aircraftCount = 0;
    _haveAble = false;

    for (int i = 0; i < 16; i++) {
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
        _aircraftData[_set][_aircraftCount].id = jsonNum(pos);

        pos = strstr(pos, "\"GAlt\":");
        if (!pos) {
            break;
        }
        _aircraftData[_set][_aircraftCount].altitude = jsonNum(pos);

        pos = strstr(pos, "\"Call\":");
        if (!pos) {
            break;
        }
        strcpy(_aircraftData[_set][_aircraftCount].callsign, jsonStr(pos));

        pos = strstr(pos, "\"Lat\":");
        if (!pos) {
            break;
        }
        _aircraftData[_set][_aircraftCount].loc.lat = jsonNum(pos);;

        pos = strstr(pos, "\"Long\":");
        if (!pos) {
            break;
        }
        _aircraftData[_set][_aircraftCount].loc.lon = jsonNum(pos);;

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
        _aircraftData[_set][_aircraftCount].speed = jsonNum(pos);;

        pos = strstr(pos, "\"Trak\":");
        if (!pos) {
            break;
        }
        _aircraftData[_set][_aircraftCount].heading = jsonNum(pos);;

        pos = strstr(pos, "\"Type\":");
        if (!pos) {
            break;
        }
        strncpy(_aircraftData[_set][_aircraftCount].typeCode, jsonStr(pos), 7);
        _aircraftData[_set][_aircraftCount].typeCode[7] = '\0';

        _aircraftData[_set][_aircraftCount].label.bmp = NULL;

        bool found = false;
        for (int i = 0; i < oldCount; i++) {
            if (_aircraftData[_set][_aircraftCount].id == _aircraftData[oldSet][i].id && _aircraftData[_set][_aircraftCount].id != -1) {
                found = true;
                strcpy(_aircraftData[_set][_aircraftCount].callsign, _aircraftData[oldSet][i].callsign);
                _aircraftData[_set][_aircraftCount].bmp = _aircraftData[oldSet][i].bmp;
                _aircraftData[_set][_aircraftCount].isAble = _aircraftData[oldSet][i].isAble;

                if (_settings.tags != 0 && _aircraftData[oldSet][i].label.bmp) {
                    bool wantBmp = true;
                    if (strcmp(_aircraftData[_set][_aircraftCount].typeCode, _aircraftData[oldSet][i].typeCode) != 0) {
                        printf("Model code for %s has changed: %s -> %s\n", _aircraftData[_set][_aircraftCount].callsign, _aircraftData[oldSet][i].typeCode, _aircraftData[_set][_aircraftCount].typeCode);
                        wantBmp = false;
                    }
                    else if (_settings.tags == 2 && _aircraftData[_set][_aircraftCount].speed != _aircraftData[oldSet][i].speed) {
                        wantBmp = false;
                    }
                    else if (_settings.tags == 2 && _aircraftData[_set][_aircraftCount].altitude != _aircraftData[oldSet][i].altitude) {
                        wantBmp = false;
                    }

                    if (wantBmp) {
                        // Move label to new
                        //printf("keep tag: %s\n", _aircraftData[oldSet][i].callsign);
                        memcpy(&_aircraftData[_set][_aircraftCount].label, &_aircraftData[oldSet][i].label, sizeof(DrawData));
                        _aircraftData[oldSet][i].label.bmp = NULL;
                    }
                }
                break;
            }
        }

        if (!found) {
            fixCallsign(_aircraftData[_set][_aircraftCount].callsign);
            _aircraftData[_set][_aircraftCount].bmp = getBmp(_aircraftData[_set][_aircraftCount].callsign, _aircraftData[_set][_aircraftCount].typeCode);
        }

        updateArea();
        _aircraftCount++;
    }

#ifdef DEBUG
    if (addMe()) {
        memcpy(&_aircraftData[_set][_aircraftCount], &me, sizeof(PosData));
        updateArea();
        _aircraftCount++;
    }
#endif

    // Add in FlightRadar data but only if Able num is completely missing
    pos = strchr(fr24Data, '#');
    while (pos) {
        pos++;
        if (addFr24(pos, &_aircraftData[_set][_aircraftCount])) {
            updateArea();
            _aircraftCount++;
        }
        pos = strchr(pos, '#');
    }

    // Cleanup old labels
    for (int i = 0; i < oldCount; i++) {
        if (_aircraftData[oldSet][i].label.bmp) {
            //printf("remove tag: %s\n", _aircraftData[oldSet][i].callsign);
            cleanupBitmap(_aircraftData[oldSet][i].label.bmp);
        }
    }

    if (_haveAble) {
        _minLoc.lat = minAbleLoc.lat;
        _minLoc.lon = minAbleLoc.lon;
        _maxLoc.lat = maxAbleLoc.lat;
        _maxLoc.lon = maxAbleLoc.lon;
    }

    if (_haveAble) {
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
