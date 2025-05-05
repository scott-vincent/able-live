#ifdef _WINDOWS
#include <windows.h>
#include <allegro5/allegro_windows.h>
#else
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>
#endif
#include <iostream>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_native_dialog.h>
#include "able_live.h"
#include "AbleClient.h"
#include "ChartFile.h"
#include "ChartCoords.h"

// Constants
const char Icon[] = "resources/images/icon.png";
const char FlightData[] = "resources/images/flight data.png";
const char AltArrow[] = "resources/images/alt arrow.png";

const char AircraftSmall[] = "resources/images/small.png";
const char AircraftJet[] = "resources/images/jet.png";
const char AircraftTurboprop[] = "resources/images/turboprop.png";
const char AircraftHeli[] = "resources/images/heli.png";
const char AircraftAble[] = "resources/images/able.png";
const char AircraftGnius[] = "resources/images/gnius.png";
const char AircraftMilitaryGnius[] = "resources/images/military_gnius.png";
const char AircraftAirliner[] = "resources/images/airliner.png";
const char AircraftHeavy[] = "resources/images/heavy.png";
const char AircraftGlider[] = "resources/images/glider.png";
const char AircraftVehicle[] = "resources/images/vehicle.png";
const char AircraftMilitarySmall[] = "resources/images/military_small.png";
const char AircraftMilitaryJet[] = "resources/images/military_jet.png";
const char AircraftMilitaryTurboprop[] = "resources/images/military_turboprop.png";
const char AircraftMilitaryHeli[] = "resources/images/military_heli.png";

const int MinScale = 5;
const int MaxScale = 200;

// Externals
extern bool _quit;
extern bool _pilotAwareQuit;
extern bool _fr24Quit;
extern bool _gniusQuit;

// Variables
double DegreesToRadians = ALLEGRO_PI / 180.0f;
ALLEGRO_FONT* _font = NULL;
ALLEGRO_FONT* _font1 = NULL;
ALLEGRO_FONT* _font2 = NULL;
ALLEGRO_DISPLAY* _display = NULL;
ALLEGRO_TIMER* _timer = NULL;
ALLEGRO_EVENT_QUEUE* _eventQueue = NULL;
ALLEGRO_MENU* _popupMenu = NULL;
int _displayWidth;
int _displayHeight;
int _winCheckDelay = 0;
DrawData _chartSource[3];
ChartData _chartSourceData[3];
Locn _border[3];
Locn _round[3];
DrawData _chart;
ChartData _chartData;
int _currentChart = -1;
DrawData _view;
AircraftDrawData _aircraft;
DrawData _flightData;
DrawData _altArrow;
DrawData _speedText;
Settings _settings;
ALLEGRO_MOUSE_STATE _mouse;
bool _mouseHidden = false;
PosData _aircraftData[MaxAircraft];
int _aircraftCount = 0;
bool _haveAble = false;
Trail _ableTrail[MaxAble];
ALLEGRO_COLOR ableColour[MaxAble];
Locn _minLoc;
Locn _maxLoc;
Locn _origMinLoc;
Locn _origMaxLoc;
char _prevAbleData[36];
int zoomDirn = 0;
Position zoomTarget;
char prevCallsign[16];
int prevAltitude = -1;
int prevSpeed = -1;
char prevLabel[128];
bool paused = false;

enum MENU_ITEMS {
    MENU_NO_TAGS = 1,
    MENU_SIMPLE_TAGS,
    MENU_FULL_TAGS,
    MENU_ADD_FLIGHTRADAR_DATA,
    MENU_SOUTHERN_ENGLAND_ONLY,
    MENU_EXCLUDE_HIGH_ALT,
    MENU_KEEP_BLACKBUSHE_ZOOMED
};

// Prototypes
void zoomView();
void doUpdate();


int showMessage(const char *message, bool isError, const char *title, bool canCancel)
{
    if (title == NULL) {
        title = "Able Live";
    }

    int msgType = 0;

    if (canCancel) {
        msgType = ALLEGRO_MESSAGEBOX_OK_CANCEL;
    }

    if (isError) {
        msgType |= ALLEGRO_MESSAGEBOX_ERROR;
    }
    else {
        msgType |= ALLEGRO_MESSAGEBOX_WARN;
    }

    return al_show_native_message_box(_display, title, "", message, NULL, msgType);
}

void cleanupBitmap(ALLEGRO_BITMAP* bmp)
{
    if (bmp != NULL) {
        try {
            al_destroy_bitmap(bmp);
        }
        catch (std::exception e) {
            // Do nothing
        }
    }
}

int checkedState(bool isChecked)
{
    if (isChecked) {
        return ALLEGRO_MENU_ITEM_CHECKED;
    }
    else {
        return ALLEGRO_MENU_ITEM_CHECKBOX;
    }
}

ALLEGRO_MENU* createMenu()
{
    ALLEGRO_MENU* menu = al_create_popup_menu();
    if (!menu) {
        printf("Failed to create popup menu\n");
        return NULL;
    }

    al_append_menu_item(menu, "No Tags", MENU_NO_TAGS, checkedState(_settings.tags == 0), NULL, NULL);
    al_append_menu_item(menu, "Simple Tags", MENU_SIMPLE_TAGS, checkedState(_settings.tags == 1), NULL, NULL);
    al_append_menu_item(menu, "Full Tags", MENU_FULL_TAGS, checkedState(_settings.tags == 2), NULL, NULL);
    al_append_menu_item(menu, "Add Flightradar24 Data", MENU_ADD_FLIGHTRADAR_DATA, checkedState(_settings.addFlightradarData), NULL, NULL);
    al_append_menu_item(menu, "Southern England Only", MENU_SOUTHERN_ENGLAND_ONLY, checkedState(_settings.southernEnglandOnly), NULL, NULL);
    //al_append_menu_item(menu, "Exclude > 6000ft", MENU_EXCLUDE_HIGH_ALT, checkedState(_settings.excludeHighAlt), NULL, NULL);
    al_append_menu_item(menu, "Keep Blackbushe Zoomed", MENU_KEEP_BLACKBUSHE_ZOOMED, checkedState(_settings.keepBlackbusheZoomed), NULL, NULL);

    return menu;
}

void showPopupMenu()
{
    if (_popupMenu) {
        al_destroy_menu(_popupMenu);
    }

    _popupMenu = createMenu();
    if (_popupMenu) {
        al_popup_menu(_popupMenu, _display);
    }
}

/// <summary>
/// Initialise global variables
/// </summary>
void initVars()
{
    _chartSource[0].bmp = NULL;
    _chartSource[1].bmp = NULL;
    _chartSource[2].bmp = NULL;
    _view.bmp = NULL;
    _flightData.bmp = NULL;
    _altArrow.bmp = NULL;
    _speedText.bmp = NULL;

    _aircraft.Small = NULL;
    _aircraft.Jet = NULL;
    _aircraft.Turboprop = NULL;
    _aircraft.Heli = NULL;
    _aircraft.Able = NULL;
    _aircraft.Gnius = NULL;
    _aircraft.Military_Gnius = NULL;
    _aircraft.Airliner = NULL;
    _aircraft.Heavy = NULL;
    _aircraft.Glider = NULL;
    _aircraft.Vehicle = NULL;
    _aircraft.Military_Small = NULL;;
    _aircraft.Military_Jet = NULL;
    _aircraft.Military_Turboprop = NULL;
    _aircraft.Military_Heli = NULL;

    // Default window position and size if no settings saved
    _settings.x = 360;
    _settings.y = 100;
    _settings.width = 1200;
    _settings.height = 800;
    _settings.framesPerSec = 0;
    _settings.tags = 2;
    _settings.addFlightradarData = true;
    _settings.southernEnglandOnly = true;
    _settings.excludeHighAlt = false;
    _settings.keepBlackbusheZoomed = false;

    _aircraftCount = 0;

    _border[0].lat = 0.004;
    _border[0].lon = 0.007;
    _round[0].lat = 0.4 / _border[0].lat;
    _round[0].lon = 0.4 / _border[0].lon;

    _border[1].lat = 0.008;
    _border[1].lon = 0.014;
    _round[1].lat = 0.4 / _border[1].lat;
    _round[1].lon = 0.4 / _border[1].lon;

    _border[2].lat = 0.010;
    _border[2].lon = 0.017;
    _round[2].lat = 0.5 / _border[2].lat;
    _round[2].lon = 0.5 / _border[2].lon;

    *_prevAbleData = '\0';

    for (int i = 0; i < MaxAble; i++) {
        _ableTrail[i].count = 0;
    }

    ableColour[0] = al_map_rgb(0xe9, 0x52, 0xff);
    ableColour[1] = al_map_rgb(0xff, 0x93, 0x52);
    ableColour[2] = al_map_rgb(0xff, 0x52, 0x52);
    ableColour[3] = al_map_rgb(0xff, 0x52, 0xd4);
    ableColour[4] = al_map_rgb(0xff, 0xd4, 0x52);
    ableColour[5] = al_map_rgb(0x52, 0xff, 0x7d);
    ableColour[6] = al_map_rgb(0x52, 0xbe, 0xff);
    ableColour[7] = al_map_rgb(0xa8, 0xff, 0x52);
    ableColour[8] = al_map_rgb(0x52, 0xff, 0xbe);
    ableColour[9] = al_map_rgb(0x67, 0x52, 0xff);
    ableColour[10] = al_map_rgb(0x67, 0xff, 0x52);
    ableColour[11] = al_map_rgb(0xe8, 0xff, 0x52);
    ableColour[12] = al_map_rgb(0x52, 0x7d, 0xff);
    ableColour[13] = al_map_rgb(0xa8, 0x52, 0xff);
    ableColour[14] = al_map_rgb(0x52, 0xff, 0xff);
    ableColour[15] = al_map_rgb(0x52, 0x7d, 0xff);
    ableColour[16] = al_map_rgb(0xff, 0x52, 0x93);
    ableColour[17] = al_map_rgb(0x52, 0xff, 0xff);
    ableColour[18] = al_map_rgb(0xff, 0x52, 0x93);
    ableColour[19] = al_map_rgb(0x52, 0x7d, 0xff);
    ableColour[20] = al_map_rgb(0xff, 0x52, 0x93);
    ableColour[21] = al_map_rgb(0x52, 0x7d, 0xff);
}

void cleanupTags()
{
    for (int i = 0; i < _aircraftCount; i++) {
        cleanupBitmap(_aircraftData[i].label.bmp);
        _aircraftData[i].label.bmp = NULL;
    }
}

void actionMenuItem(MENU_ITEMS menuItem)
{
    switch (menuItem) {

    case MENU_NO_TAGS:
        _settings.tags = 0;
        saveSettings();
        cleanupTags();
        break;

    case MENU_SIMPLE_TAGS:
        _settings.tags = 1;
        saveSettings();
        cleanupTags();
        break;

    case MENU_FULL_TAGS:
        _settings.tags = 2;
        saveSettings();
        cleanupTags();
        break;

    case MENU_ADD_FLIGHTRADAR_DATA:
        _settings.addFlightradarData = !_settings.addFlightradarData;
        saveSettings();
        break;

    case MENU_SOUTHERN_ENGLAND_ONLY:
        _settings.southernEnglandOnly = !_settings.southernEnglandOnly;
        saveSettings();
        break;

    case MENU_EXCLUDE_HIGH_ALT:
        _settings.excludeHighAlt = !_settings.excludeHighAlt;
        saveSettings();
        break;

    case MENU_KEEP_BLACKBUSHE_ZOOMED:
        _settings.keepBlackbusheZoomed = !_settings.keepBlackbusheZoomed;
        saveSettings();
        break;
    }
}

ALLEGRO_FONT* loadFont(const char* name, int size)
{
    ALLEGRO_FONT* font = NULL;

    char path[256];
    sprintf(path, "%s/%s", FontsDir, name);

    if (!(font = al_load_ttf_font(path, size, 0))) {
        printf("Failed to load font %s\n", name);
    }

    return font;
}

/// <summary>
/// Initialise Allegro
/// </summary>
bool init()
{
    if (!al_init()) {
        printf("Failed to initialise Allegro\n");
        return false;
    }

    if (!al_init_primitives_addon()) {
        printf("Failed to initialise primitives\n");
        return false;
    }

    if (!al_init_font_addon()) {
        printf("Failed to initialise font\n");
        return false;
    }

    if (!al_init_ttf_addon()) {
        printf("Failed to initialise ttf\n");
        return false;
    }

    if (!al_init_image_addon()) {
        printf("Failed to initialise image\n");
        return false;
    }

    if (!al_init_native_dialog_addon()) {
        printf("Failed to initialise native dialog\n");
        return false;
    }

    if (!al_install_keyboard()) {
        printf("Failed to initialise keyboard\n");
        return false;
    }

    if (!al_install_mouse()) {
        printf("Failed to initialise mouse\n");
        return false;
    }

    if (!(_eventQueue = al_create_event_queue())) {
        printf("Failed to create event queue\n");
        return false;
    }

    if (!(_font = al_create_builtin_font())) {
        printf("Failed to create font\n");
        return false;
    }

    if (!(_font1 = loadFont("arial.ttf", 10))) {
        return false;
    }

    if (!(_font2 = loadFont("arialbd.ttf", 14))) {
        return false;
    }

    al_set_new_window_title("Able Live");

#ifdef _WINDOWS
    al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);

    // Turn on vsync
    al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_REQUIRE);
#else
    al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW | ALLEGRO_FRAMELESS | ALLEGRO_GTK_TOPLEVEL);
    //al_set_new_display_flags(ALLEGRO_RESIZABLE | ALLEGRO_GTK_TOPLEVEL);
    _settings.x = 0;
    _settings.y = 0;
    _settings.width = 1920;
    _settings.height = 1080;
#endif

    // Create window
    if ((_display = al_create_display(_settings.width, _settings.height)) == NULL) {
        printf("Failed to create display\n");
        return false;
    }

    // Add icon
    ALLEGRO_BITMAP* icon = al_load_bitmap(Icon);
    if (icon) {
        al_set_display_icon(_display, icon);
        al_destroy_bitmap(icon);
    }
    else {
        printf("Failed to load icon\n");
        return false;
    }

#ifdef _WINDOWS
    // Make sure window isn't minimised
    HWND _displayWindow = al_get_win_window_handle(_display);
    ShowWindow(_displayWindow, SW_SHOWNORMAL);

    // Make sure window is within monitor bounds
    bool visible = false;
    ALLEGRO_MONITOR_INFO monitorInfo;
    int numMonitors = al_get_num_video_adapters();
    for (int i = 0; i < numMonitors; i++) {
        al_get_monitor_info(i, &monitorInfo);

        if (_settings.x >= monitorInfo.x1 && _settings.x < monitorInfo.x2
            && _settings.y >= monitorInfo.y1 && _settings.y < monitorInfo.y2)
        {
            visible = true;
            break;
        }
    }

    if (!visible && numMonitors > 0) {
        _settings.x = monitorInfo.x1;
        _settings.y = monitorInfo.y1;
    }

    // Position window
    al_set_window_position(_display, _settings.x, _settings.y);
#else
    al_inhibit_screensaver(true);
    al_hide_mouse_cursor(_display);
    _mouseHidden = true;
#endif

    _displayWidth = al_get_display_width(_display);
    _displayHeight = al_get_display_height(_display);

    al_register_event_source(_eventQueue, al_get_keyboard_event_source());
    al_register_event_source(_eventQueue, al_get_mouse_event_source());
    al_register_event_source(_eventQueue, al_get_display_event_source(_display));
    al_register_event_source(_eventQueue, al_get_default_menu_event_source());

    if (!(_timer = al_create_timer(1.0 / _settings.framesPerSec))) {
        printf("Failed to create timer\n");
        return false;
    }

    al_register_event_source(_eventQueue, al_get_timer_event_source(_timer));

    return true;
}

/// <summary>
/// Cleanup Allegro
/// </summary>
void cleanup()
{
    cleanupBitmap(_chartSource[0].bmp);
    cleanupBitmap(_chartSource[1].bmp);
    cleanupBitmap(_chartSource[2].bmp);
    cleanupBitmap(_view.bmp);
    cleanupBitmap(_flightData.bmp);
    cleanupBitmap(_altArrow.bmp);
    cleanupBitmap(_speedText.bmp);

    cleanupBitmap(_aircraft.Small);
    cleanupBitmap(_aircraft.Jet);
    cleanupBitmap(_aircraft.Turboprop);
    cleanupBitmap(_aircraft.Heli);
    cleanupBitmap(_aircraft.Able);
    cleanupBitmap(_aircraft.Gnius);
    cleanupBitmap(_aircraft.Military_Gnius);
    cleanupBitmap(_aircraft.Airliner);
    cleanupBitmap(_aircraft.Heavy);
    cleanupBitmap(_aircraft.Glider);
    cleanupBitmap(_aircraft.Vehicle);
    cleanupBitmap(_aircraft.Military_Small);
    cleanupBitmap(_aircraft.Military_Jet);
    cleanupBitmap(_aircraft.Military_Turboprop);
    cleanupBitmap(_aircraft.Military_Heli);

    cleanupTags();

    if (_popupMenu) {
        al_destroy_menu(_popupMenu);
    }

    if (_timer) {
        al_destroy_timer(_timer);
    }

    if (_eventQueue) {
        al_destroy_event_queue(_eventQueue);
    }

    if (_font) {
        al_destroy_font(_font);
    }

    if (_font1) {
        al_destroy_font(_font1);
    }

    if (_font2) {
        al_destroy_font(_font2);
    }

    if (_display) {
        al_destroy_display(_display);
        al_inhibit_screensaver(false);
    }
}

bool loadImage(const char* filename, DrawData* draw)
{
    draw->bmp = al_load_bitmap(filename);
    if (!draw->bmp) {
        printf("Missing file: %s\n", filename);
        return false;
    }

    draw->width = al_get_bitmap_width(draw->bmp);
    draw->height = al_get_bitmap_height(draw->bmp);
    draw->x = draw->width / 2;
    draw->y = draw->height / 2;

    return true;
}

bool initCharts()
{
    if (!loadImage(SmallChart, &_chartSource[0]) || !loadCalibrationData(SmallChart, &_chartSourceData[0])) {
        return false;
    }

    if (!loadImage(MediumChart, &_chartSource[1]) || !loadCalibrationData(MediumChart, &_chartSourceData[1])) {
        return false;
    }

    if (!loadImage(LargeChart, &_chartSource[2]) || !loadCalibrationData(LargeChart, &_chartSourceData[2])) {
        return false;
    }

    return true;
}

bool initAircraft()
{
    if (!(_aircraft.Small = al_load_bitmap(AircraftSmall))) {
        printf("Missing file: %s\n", AircraftSmall);
        return false;
    }

    if (!(_aircraft.Jet = al_load_bitmap(AircraftJet))) {
        printf("Missing file: %s\n", AircraftJet);
        return false;
    }

    if (!(_aircraft.Turboprop = al_load_bitmap(AircraftTurboprop))) {
        printf("Missing file: %s\n", AircraftTurboprop);
        return false;
    }

    if (!(_aircraft.Heli = al_load_bitmap(AircraftHeli))) {
        printf("Missing file: %s\n", AircraftHeli);
        return false;
    }

    if (!(_aircraft.Able = al_load_bitmap(AircraftAble))) {
        printf("Missing file: %s\n", AircraftAble);
        return false;
    }

    if (!(_aircraft.Gnius = al_load_bitmap(AircraftGnius))) {
        printf("Missing file: %s\n", AircraftGnius);
        return false;
    }

    if (!(_aircraft.Military_Gnius = al_load_bitmap(AircraftMilitaryGnius))) {
        printf("Missing file: %s\n", AircraftMilitaryGnius);
        return false;
    }

    if (!(_aircraft.Airliner = al_load_bitmap(AircraftAirliner))) {
        printf("Missing file: %s\n", AircraftAirliner);
        return false;
    }

    if (!(_aircraft.Heavy = al_load_bitmap(AircraftHeavy))) {
        printf("Missing file: %s\n", AircraftHeavy);
        return false;
    }

    if (!(_aircraft.Glider = al_load_bitmap(AircraftGlider))) {
        printf("Missing file: %s\n", AircraftGlider);
        return false;
    }

    if (!(_aircraft.Vehicle = al_load_bitmap(AircraftVehicle))) {
        printf("Missing file: %s\n", AircraftVehicle);
        return false;
    }

    if (!(_aircraft.Military_Small = al_load_bitmap(AircraftMilitarySmall))) {
        printf("Missing file: %s\n", AircraftMilitarySmall);
        return false;
    }

    if (!(_aircraft.Military_Jet = al_load_bitmap(AircraftMilitaryJet))) {
        printf("Missing file: %s\n", AircraftMilitaryJet);
        return false;
    }

    if (!(_aircraft.Military_Turboprop = al_load_bitmap(AircraftMilitaryTurboprop))) {
        printf("Missing file: %s\n", AircraftMilitaryTurboprop);
        return false;
    }

    if (!(_aircraft.Military_Heli = al_load_bitmap(AircraftMilitaryHeli))) {
        printf("Missing file: %s\n", AircraftMilitaryHeli);
        return false;
    }

    _aircraft.width = al_get_bitmap_width(_aircraft.Small);
    _aircraft.height = al_get_bitmap_height(_aircraft.Small);
    _aircraft.x = _aircraft.width / 2;
    _aircraft.y = _aircraft.height / 2;

    return true;
}

bool initFlightData()
{
    return loadImage(FlightData, &_flightData);
}

bool initAltArrow()
{
    return loadImage(AltArrow, &_altArrow);
}

/// <summary>
/// Chart 0 = Small, 1 = Medium, 2 = Large
/// </summary>
void switchChart(int num)
{
    if (_currentChart == num) {
        return;
    }

#ifdef DEBUG
    switch (num) {
    case 0: printf("Small chart\n"); break;
    case 1: printf("Medium chart\n"); break;
    default: printf("Large chart\n"); break;
    }
#endif

    _currentChart = num;
        
    memcpy(&_chart, &_chartSource[num], sizeof(_chart));
    memcpy(&_chartData, &_chartSourceData[num], sizeof(_chartData));

    _chart.scale = 100;
    doUpdate();
}

void expandBorder(int num, Locn* min, Locn* max)
{
    min->lat = floor(_round[num].lat * (_minLoc.lat - _border[num].lat)) / _round[num].lat;
    min->lon = floor(_round[num].lon * (_minLoc.lon - _border[num].lon)) / _round[num].lon;
    max->lat = ceil(_round[num].lat * (_maxLoc.lat + _border[num].lat)) / _round[num].lat;
    max->lon = ceil(_round[num].lon * (_maxLoc.lon + _border[num].lon)) / _round[num].lon;
}

double zoom(ChartData* chartData, Locn* minLoc, Locn* maxLoc)
{
    double borderLat = maxLoc->lat - minLoc->lat;
    double borderLon = maxLoc->lon - minLoc->lon;
    double chartLat = abs(chartData->lat[1] - chartData->lat[0]);
    double chartLon = abs(chartData->lon[1] - chartData->lon[0]);

    double zoom = borderLat / chartLat;
    if (zoom < borderLon / chartLon) {
        zoom = borderLon / chartLon;
    }

    return zoom;
}

void initView()
{
    memcpy(&_origMinLoc, &_minLoc, sizeof(Locn));
    memcpy(&_origMaxLoc, &_maxLoc, sizeof(Locn));

    Locn minSmall, maxSmall;
    expandBorder(0, &minSmall, &maxSmall);
    double smallZoom = zoom(&_chartSourceData[0], &minSmall, &maxSmall);

    Locn minMedium, maxMedium;
    expandBorder(1, &minMedium, &maxMedium);
    double mediumZoom = zoom(&_chartSourceData[1], &minMedium, &maxMedium);

    if (minSmall.lat >= MinLatSmall && minSmall.lon >= MinLonSmall && maxSmall.lat <= MaxLatSmall && maxSmall.lon <= MaxLonSmall && smallZoom < 0.65) {
        switchChart(0);
        //printf("border left: %f, right: %f, top: %f, bottom: %f\n", minSmall.lon - _minLoc.lon, maxSmall.lon - _maxLoc.lon, maxSmall.lat - _maxLoc.lat, minSmall.lat - _minLoc.lat);
        memcpy(&_minLoc, &minSmall, sizeof(Locn));
        memcpy(&_maxLoc, &maxSmall, sizeof(Locn));
    }
    else if (minMedium.lat >= MinLatMedium && minMedium.lon >= MinLonMedium && maxMedium.lat <= MaxLatMedium && maxMedium.lon <= MaxLonMedium && mediumZoom < 0.81) {
        switchChart(1);
        //printf("border left: %f, right: %f, top: %f, bottom: %f\n", minMedium.lon - _minLoc.lon, maxMedium.lon - _maxLoc.lon, maxMedium.lat - _maxLoc.lat, minMedium.lat - _minLoc.lat);
        memcpy(&_minLoc, &minMedium, sizeof(Locn));
        memcpy(&_maxLoc, &maxMedium, sizeof(Locn));
    }
    else {
        switchChart(2);
        Locn minLarge, maxLarge;
        expandBorder(2, &minLarge, &maxLarge);
        //printf("border left: %f, right: %f, top: %f, bottom: %f\n", minLarge.lon - _minLoc.lon, maxLarge.lon - _maxLoc.lon, maxLarge.lat - _maxLoc.lat, minLarge.lat - _minLoc.lat);
        memcpy(&_minLoc, &minLarge, sizeof(Locn));
        memcpy(&_maxLoc, &maxLarge, sizeof(Locn));
    }

    // Position centre of map
    Locn locCentre;
    locCentre.lat = (_minLoc.lat + _maxLoc.lat) / 2.0;
    locCentre.lon = (_minLoc.lon + _maxLoc.lon) / 2.0;

    Position centrePos;
    locationToChartPos(&locCentre, &centrePos);

    _chart.x = centrePos.x;
    _chart.y = centrePos.y;

    // Zoom map
    Position zoomCurrent;
    displayToChartPos(0, 0, &zoomCurrent);

    Locn targetLoc;
    targetLoc.lat = _maxLoc.lat;
    targetLoc.lon = _minLoc.lon;
    locationToChartPos(&targetLoc, &zoomTarget);

    if (zoomTarget.x < zoomCurrent.x || zoomTarget.y < zoomCurrent.y) {
        zoomDirn = 1;
    }
    else if (zoomTarget.x > zoomCurrent.x && zoomTarget.y > zoomCurrent.y) {
        zoomDirn = -1;
    }
    else {
        zoomDirn = 0;
    }

    // Find optimum zoom
    while (zoomDirn != 0) {
        zoomView();
        doUpdate();
    }
}

void zoomView()
{
    // Zoom map
    double newScale;
    Position zoomCurrent;
    displayToChartPos(0, 0, &zoomCurrent);

    if (zoomDirn == 1) {
        if (zoomTarget.x > zoomCurrent.x && zoomTarget.y > zoomCurrent.y) {
            zoomDirn = 0;
            _chart.scale += 1;
            //printf("Zoom finished - scale: %f\n", _chart.scale);
        }
        else {
            newScale = _chart.scale - 1;
            if (newScale < MinScale) {
                //printf("Zoom min - scale: %f\n", _chart.scale);
                zoomDirn = 0;
            }
            else {
                _chart.scale = newScale;
                //printf("Zoom in - scale: %f\n", _chart.scale);
            }
        }
    }
    else if (zoomDirn == -1) {
        if (zoomTarget.x < zoomCurrent.x || zoomTarget.y < zoomCurrent.y) {
            zoomDirn = 0;
            _chart.scale -= 1;
            //printf("Zoom finished - scale: %f\n", _chart.scale);
        }
        else {
            newScale = _chart.scale + 1;
            if (newScale > MaxScale) {
                //printf("Zoom max - scale: %f\n", _chart.scale);
                zoomDirn = 0;
            }
            else {
                _chart.scale = newScale;
                //printf("Zoom out - scale: %f\n", _chart.scale);
            }
        }
    }
}

void createSpeedText(int speed)
{
    char text[16];
    sprintf(text, "%d kts", speed);

    _speedText.width = 60;
    _speedText.height = 16;

    cleanupBitmap(_speedText.bmp);
    _speedText.bmp = al_create_bitmap(_speedText.width, _speedText.height);

    al_set_target_bitmap(_speedText.bmp);
    al_clear_to_color(al_map_rgb(0x83, 0x83, 0x99));

    al_draw_text(_font2, al_map_rgb(0xf1, 0xf1, 0xf1), 0, 0, 0, text);
    al_set_target_backbuffer(_display);
}

void drawArea()
{
    double x1, y1, x2, y2;
    locationToDisplay(&_minLoc, &x1, &y1);
    locationToDisplay(&_maxLoc, &x2, &y2);

    ALLEGRO_COLOR colour = al_map_rgb(0xc0, 0x0, 0xc0);

    al_draw_line(x1, y1, x2, y1, colour, 3);
    al_draw_line(x2, y1, x2, y2, colour, 3);
    al_draw_line(x2, y2, x1, y2, colour, 3);
    al_draw_line(x1, y2, x1, y1, colour, 3);

    locationToDisplay(&_origMinLoc, &x1, &y1);
    locationToDisplay(&_origMaxLoc, &x2, &y2);

    al_draw_line(x1, y1, x2, y1, colour, 3);
    al_draw_line(x2, y1, x2, y2, colour, 3);
    al_draw_line(x2, y2, x1, y2, colour, 3);
    al_draw_line(x1, y2, x1, y1, colour, 3);
}

void createTag(PosData* posData)
{
    ALLEGRO_COLOR background = al_map_rgb(0x83, 0x83, 0x99);
    ALLEGRO_COLOR colour = al_map_rgb(0xf1, 0xf1, 0xf1);

    posData->label.width = al_get_text_width(_font1, posData->callsign) + 2;

    char fullText[16];
    if (_settings.tags == 1) {
        posData->label.height = 22;
    }
    else {
        sprintf(fullText, "%d  %d", posData->speed, posData->altitude);
        int width = al_get_text_width(_font1, fullText) + 2;
        if (posData->label.width < width) {
            posData->label.width = width;
        }
        posData->label.height = 32;
    }

    posData->label.bmp = al_create_bitmap(posData->label.width, posData->label.height);

    al_set_target_bitmap(posData->label.bmp);
    al_clear_to_color(background);

    al_draw_text(_font1, colour, 1, 0, 0, posData->callsign);
    al_draw_text(_font1, colour, 1, 10, 0, posData->typeCode);

    if (_settings.tags == 2) {
        al_draw_text(_font1, colour, 1, 20, 0, fullText);
    }

    al_set_target_backbuffer(_display);
}

void drawAircraft(int idx)
{
    if (_settings.excludeHighAlt && !_aircraftData[idx].isAble && _aircraftData[idx].altitude > 6000) {
        return;
    }

    double x, y;
    locationToDisplay(&_aircraftData[idx].loc, &x, &y);

    int bounds = 25;
    if (x < -bounds || y < -bounds || x > _displayWidth + bounds || y > _displayHeight + bounds) {
        return;
    }

    if (_settings.tags > 0) {
        if (_aircraftData[idx].label.bmp == NULL) {
            createTag(&_aircraftData[idx]);
        }

        // Draw tag
        al_draw_bitmap(_aircraftData[idx].label.bmp, x - _aircraftData[idx].label.width / 2, y + 20, 0);
    }

    // Draw aircraft
    double scale = 0.18;
    al_draw_scaled_rotated_bitmap(_aircraftData[idx].bmp, _aircraft.x, _aircraft.y, x, y, scale, scale, _aircraftData[idx].heading * DegreesToRadians, 0);
}

void drawTrail(Trail *trail, int num)
{
    int prevX = 0;
    int prevY = 0;

    double x1, y1, x2, y2;
    locationToDisplay(&trail->loc[0], &x1, &y1);

    for (int i = 1; i < trail->count; i++) {
        locationToDisplay(&trail->loc[i], &x2, &y2);

        al_draw_line(round(x1), round(y1), round(x2), round(y2), ableColour[num], 2);

        prevX = x1;
        prevY = y1;
        x1 = x2;
        y1 = y2;
    }

    if (num > 15 && trail->count > 1) {
        // If sim trail has jumped then sim was restarted so delete old trail
        int xDiff = x1 - prevX;
        int yDiff = y1 - prevY;
        if (abs(xDiff) > 150 || abs(yDiff) > 150) {
            trail->count = 0;
        }
    }
}

void render()
{
    // Draw chart
    al_draw_scaled_bitmap(_chart.bmp, _view.x, _view.y, _view.width, _view.height, 0, 0, _displayWidth, _displayHeight, 0);

#ifdef DEBUG
    drawArea();
#endif

    if (_haveAble) {
        for (int i = 0; i < MaxAble; i++) {
            if (_ableTrail[i].count > 0) {
                drawTrail(&_ableTrail[i], i);
            }
        }
    }

#ifdef COLOUR_DEBUG
    _ableTrail[17].count = 2;
    _ableTrail[17].loc[0].lon = -1.0;
    _ableTrail[17].loc[1].lon = -0.5;

    for (int i = 0; i < MaxAble; i++) {
        _ableTrail[17].loc[0].lat = 51.45 - i * 0.01;
        _ableTrail[17].loc[1].lat = _ableTrail[15].loc[0].lat;
        drawTrail(&_ableTrail[15], i);
    }
    _ableTrail[17].count = 0;
#endif

    for (int i = 0; i < _aircraftCount; i++) {
        drawAircraft(i);
    }
}

/// <summary>
/// Initialise everything
/// </summary>
bool doInit()
{
    if (!initCharts()) {
        return false;
    }

    if (!initAircraft()) {
        return false;
    }

    if (!initFlightData()) {
        return false;
    }

    if (!initAltArrow()) {
        return false;
    }

    return true;
}

/// <summary>
/// Render the next frame
/// </summary>
void doRender()
{
    // Clear background
    al_clear_to_color(al_map_rgb(0, 0, 0));

    // Draw chart and aircraft
    render();

    // Allegro can detect window resize but not window move so do it here.
    // Only need to check once every second.
    if (_winCheckDelay > 0) {
        _winCheckDelay--;
    }
    else {
        _winCheckDelay = _settings.framesPerSec;
        int winX, winY;
        al_get_window_position(_display, &winX, &winY);
        if (_settings.x != winX || _settings.y != winY)
        {
            _settings.x = winX;
            _settings.y = winY;
            saveSettings();
        }
    }
}

/// <summary>
/// Update everything before the next frame
/// </summary>
void doUpdate()
{
    if (_chart.scale < MinScale) {
        _chart.scale = MinScale;
    }
    else if (_chart.scale > MaxScale) {
        _chart.scale = MaxScale;
    }

    // Want zoom to be exponential
    double zoomScale = _chart.scale * _chart.scale / 10000.0;
    if (_view.scale != zoomScale) {
        _view.scale = zoomScale;
    }

    // Calculate view, i.e. part of chart that is visible
    _view.x = _chart.x + (0 - _displayWidth / 2.0) / _view.scale;
    _view.y = _chart.y + (0 - _displayHeight / 2.0) / _view.scale;
    _view.width = _displayWidth / _view.scale;
    _view.height = _displayHeight / _view.scale;
}

/// <summary>
/// Handle keypress
/// </summary>
bool doKeypress(ALLEGRO_EVENT* event, bool isDown)
{
    switch (event->keyboard.keycode) {
    case ALLEGRO_KEY_ESCAPE:
        _quit = true;
        break;
#ifdef DEBUG
    case ALLEGRO_KEY_SPACE:
        if (isDown) {
            paused = true;
        }
        else {
            paused = false;
        }
#endif
        break;
    }

    return false;
}

/// <summary>
/// Handle mouse button press or release
/// </summary>
bool doMouseButton(ALLEGRO_EVENT* event, bool isPress)
{
    static Position clickedPos;

    al_get_mouse_state(&_mouse);

    if (_mouseHidden) {
        _mouseHidden = false;
        al_show_mouse_cursor(_display);
    }

    if (event->mouse.button == 2) {
        if (!isPress) {
            // Show popup menu on release
            showPopupMenu();
        }
    }

    return false;
}

long getMillis()
{
#ifdef _WINDOWS
    SYSTEMTIME now;
    GetSystemTime(&now);
    return now.wHour * 3600000 + now.wMinute * 60000 + now.wSecond * 1000 + now.wMilliseconds;
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec % 86400) * 1000 + now.tv_usec / 1000;
#endif
}

long milliDiff(int millis1, int millis2)
{
    if (millis1 <= millis2) {
        if (millis1 == 0) {
            return 86400000;
        }
        return millis2 - millis1;
    }

    return (millis2 + 86400000) - millis1;
}

///
/// main
///
void showChart()
{
    initVars();
    loadSettings();

    if (!init()) {
        cleanup();
        return;
    }

    if (!doInit()) {
        cleanup();
        return;
    }

    GetLiveData();
    switchChart(1);
    initView();

    bool redraw = true;
    ALLEGRO_EVENT event;

    al_start_timer(_timer);

    long now;
    long lastFetch = 0;
    long lastSuccess = getMillis();

    while (!_quit && !_pilotAwareQuit && !_fr24Quit && !_gniusQuit) {
        al_wait_for_event(_eventQueue, &event);

        switch (event.type) {
        case ALLEGRO_EVENT_TIMER:
            //doUpdate();
            redraw = true;
            break;

        case ALLEGRO_EVENT_MENU_CLICK:
            actionMenuItem((MENU_ITEMS)event.user.data1);
            break;

        case ALLEGRO_EVENT_KEY_DOWN:
            if (doKeypress(&event, true)) {
                redraw = true;
            }
            break;

        case ALLEGRO_EVENT_KEY_UP:
            if (doKeypress(&event, false)) {
                redraw = true;
            }
            break;

        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            if (doMouseButton(&event, true)) {
                redraw = true;
            }
            break;

        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            if (doMouseButton(&event, false)) {
                redraw = true;
            }
            break;

        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            _quit = true;
            break;

        case ALLEGRO_EVENT_DISPLAY_RESIZE:
            _displayWidth = event.display.width;
            _displayHeight = event.display.height;
            al_acknowledge_resize(_display);
            _settings.width = _displayWidth;
            _settings.height = _displayHeight;
            saveSettings();
            initView();
            break;
        }

        if (redraw && al_is_event_queue_empty(_eventQueue) && !_quit) {
            doRender();
            al_flip_display();
            redraw = false;
        }

        long now = getMillis();
        if (milliDiff(lastFetch, now) > 1200 && !paused) {
            lastFetch = now;
            if (GetLiveData()) {
                lastSuccess = now;
                _settings.excludeHighAlt = _haveAble;
                initView();
            }
            else if (milliDiff(lastSuccess, now) > 300000) {
                printf("Too many errors: quitting\n");
                _quit = true;
            }
            else if (milliDiff(lastSuccess, now) > 30000) {
                cleanupTags();
                _aircraftCount = 0;
            }
        }
    }

    cleanup();
}
