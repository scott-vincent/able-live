#pragma once
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

// Comment out the following lines when not debugging
//#define DEBUG
//#define COLOUR_DEBUG

// Constants
#ifdef _WINDOWS
const char WriteDataFile[] = "pilotaware_data";
#else
const char WriteDataFile[] = "/mem/pilotaware_data";    // Send PilotAware data to AbleDisplay
#endif
const char PilotAware_Url[] = "../../pilotaware.uri";       // Fetch PilotAware data from ognpaw
const char AbleDisplay_Url[] = "../../able-display.uri";    // Fetch FR24 data from AbleDisplay
const int MaxPilotAwareData = 262144;
const int MaxFr24Data = 512;
const int MaxGniusData = 512;
const int MaxAircraft = 256;
const int MaxAble = 22;
const char ImagesDir[] = "resources/images";
const char FontsDir[] = "resources/fonts";
const char SmallChart[] = "resources/maps/Small.png";
const char MediumChart[] = "resources/maps/Medium.png";
const char LargeChart[] = "resources/maps/Large.png";
const double BlackbusheLat = 51.324;
const double BlackbusheLon = -0.845;
const double MinLatSmall = 51.17;
const double MinLonSmall = -1.09;
const double MaxLatSmall = 51.46;
const double MaxLonSmall = -0.62;  // Area = 0.29 lat by 0.47 lon
const double MinLatMedium = 50.75;
const double MinLonMedium = -1.59;
const double MaxLatMedium = 51.69;
const double MaxLonMedium = 0.09;  // Area = 0.94 lat by 1.68 lon
const double MinLatLarge = 49.92;
const double MinLonLarge = -5.81;
const double MaxLatLarge = 54.51;
const double MaxLonLarge = 1.79;   // Area = 4.59 lat by 7.60 lon

const int DefaultFPS = 3;
const double RadiansToDegrees = 180.0f / ALLEGRO_PI;

const char Jets[] = "_AST_BE4_C25_C51_C52_C55_C56_C68_C75_CL3_CL6_CRJ_E13_E14_E35_E50_E55_EA5_F2T_F90_FA2_FA7_FA8_G28_GA5_GA6_GAL_GL5_GL7_GLE_GLF_H25_HDJ_LJ3_LJ7_PC2_PRM_";
const char Turboprops[] = "_AT4_AT7_B35_BE2_D22_D32_DH8_DHC_P18_SC7_SF3_C303_";
const char Helis[] = "_A10_A13_A16_A18_AS5_B06_B50_B17_CLO_EC3_EC4_EC5_EC7_EXP_G2C_MM1_R22_R44_R66_S76_WAS_";
const char Airliners[] = "_A20_A21_A30_A31_A32_A33_BCS_B38_B73_B75_B76_E19_E29_E75_";
const char Heavys[] = "_A34_A35_A38_A3S_B74_B77_B78_";

const char Military_Jets[] = "_SB3_F15_HAW_HUN_REDAR_";  // REDAR = Red Arrow
const char Military_Smalls[] = "_SPI_GTCHI_P51_FUR_";    // GTCHI = Spitfire
const char Military_Turboprops[] = "_A40_B52_C13_C30_E39_E3C_K35_LAN_";
const char Military_Turboprop2s[] = "_C17_";             // Matches full model only (to stop it matching, e.g. C172)
const char Military_Helis[] = "_H47_H64_LYN_PUM_UGLY1_"; // UGLY = Apache

struct Position {
    int x;
    int y;
};

struct Locn {
    double lat;
    double lon;
};

struct DrawData {
    ALLEGRO_BITMAP* bmp;
    int x;
    int y;
    int width;
    int height;
    double scale;
};

struct AircraftDrawData {
    ALLEGRO_BITMAP* Small;
    ALLEGRO_BITMAP* Jet;
    ALLEGRO_BITMAP* Turboprop;
    ALLEGRO_BITMAP* Heli;
    ALLEGRO_BITMAP* Able;
    ALLEGRO_BITMAP* Gnius;
    ALLEGRO_BITMAP* Military_Gnius;
    ALLEGRO_BITMAP* Airliner;
    ALLEGRO_BITMAP* Heavy;
    ALLEGRO_BITMAP* Glider;
    ALLEGRO_BITMAP* Vehicle;
    ALLEGRO_BITMAP* Military_Small;
    ALLEGRO_BITMAP* Military_Jet;
    ALLEGRO_BITMAP* Military_Turboprop;
    ALLEGRO_BITMAP* Military_Heli;
    int x;
    int y;
    int width;
    int height;
    double scale;
};

struct ChartData {
    int x[2];
    int y[2];
    double lat[2];
    double lon[2];
};

struct Settings {
    int x;
    int y;
    int width;
    int height;
    int framesPerSec;
    char location[256];
    int tags;
    bool addFlightradarData;
    bool southernEnglandOnly;
    bool excludeHighAlt;
    bool keepBlackbusheZoomed;
};

struct CalibratedData {
    char filename[256];
    ChartData data;
};

struct PosData {
    int id;
    char callsign[16];
    char typeCode[8];
    ALLEGRO_BITMAP* bmp;
    Locn loc;
    int heading;
    int altitude;
    int speed;
    DrawData label;
    bool isAble;
};

struct Trail {
    int count;
    Locn loc[6000];
    time_t lastUpdate;
};

// Prototypes
int showMessage(const char* message, bool isError, const char* title = NULL, bool canCancel = false);
