#ifdef _WINDOWS
#include <windows.h>
#endif
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include "ChartCoords.h"

const double RadiusOfEarthNm = 3440.1;

// Externals
extern double DegreesToRadians;
extern int _displayWidth;
extern int _displayHeight;
extern DrawData _chart;
extern DrawData _view;
extern ChartData _chartData;

/// <summary>
/// Get position on zoomed chart of display's top left corner
/// </summary>
void getDisplayPos(Position* display)
{
    display->x = _chart.x * _view.scale - _displayWidth / 2;
    display->y = _chart.y * _view.scale - _displayHeight / 2;
}

/// <summary>
/// Convert display position to chart position
/// </summary>
void displayToChartPos(int displayX, int displayY, Position* chart)
{
    Position displayPos;
    getDisplayPos(&displayPos);

    int zoomedX = displayPos.x + displayX;
    int zoomedY = displayPos.y + displayY;

    chart->x = zoomedX / _view.scale;
    chart->y = zoomedY / _view.scale;
}

/// <summary>
/// Convert chart position to display position.
/// Returned position may be outside the display area.
/// </summary>
void chartToDisplayPos(int chartX, int chartY, Position* display)
{
    Position displayPos;
    getDisplayPos(&displayPos);

    display->x = chartX * _view.scale - displayPos.x;
    display->y = chartY * _view.scale - displayPos.y;
}

/// <summary>
/// Convert location to chart position. Chart must be calibrated.
/// </summary>
void locationToChartPos(Locn* loc, Position* pos)
{
    double lonCalibDiff = _chartData.lon[1] - _chartData.lon[0];
    double lonDiff = loc->lon - _chartData.lon[0];
    double xScale = lonDiff / lonCalibDiff;
    int xCalibDiff = _chartData.x[1] - _chartData.x[0];
    pos->x = _chartData.x[0] + xCalibDiff * xScale;

    double latCalibDiff = _chartData.lat[1] - _chartData.lat[0];
    double latDiff;

    if (abs(latCalibDiff) < 2) {
        // Assume linear lat scale
        latDiff = loc->lat - _chartData.lat[0];
    }
    else {
        // Account for map projection (lat stretches towards poles).
        // Use rough and ready formula y = ((lat + 26.4)^2 / 7.9) - 60
        double lat0 = (pow(_chartData.lat[0] + 26.4, 2) / 7.9) - 60;
        double lat1 = (pow(_chartData.lat[1] + 26.4, 2) / 7.9) - 60;
        double yPos = (pow(loc->lat + 26.4, 2) / 7.9) - 60;

        latCalibDiff = lat1 - lat0;
        latDiff = yPos - lat0;
    }

    double yScale = latDiff / latCalibDiff;
    int yCalibDiff = _chartData.y[1] - _chartData.y[0];
    pos->y = _chartData.y[0] + yCalibDiff * yScale;
}

void locationToDisplay(Locn* loc, double* x, double* y)
{
    double chartX, chartY;

    double lonCalibDiff = _chartData.lon[1] - _chartData.lon[0];
    double lonDiff = loc->lon - _chartData.lon[0];
    double xScale = lonDiff / lonCalibDiff;
    int xCalibDiff = _chartData.x[1] - _chartData.x[0];
    chartX = _chartData.x[0] + xCalibDiff * xScale;

    double latCalibDiff = _chartData.lat[1] - _chartData.lat[0];
    double latDiff;

    if (abs(latCalibDiff) < 2) {
        // Assume linear lat scale
        latDiff = loc->lat - _chartData.lat[0];
    }
    else {
        // Account for map projection (lat stretches towards poles).
        // Use rough and ready formula y = ((lat + 26.4)^2 / 7.9) - 60
        double lat0 = (pow(_chartData.lat[0] + 26.4, 2) / 7.9) - 60;
        double lat1 = (pow(_chartData.lat[1] + 26.4, 2) / 7.9) - 60;
        double yPos = (pow(loc->lat + 26.4, 2) / 7.9) - 60;

        latCalibDiff = lat1 - lat0;
        latDiff = yPos - lat0;
    }

    double yScale = latDiff / latCalibDiff;
    int yCalibDiff = _chartData.y[1] - _chartData.y[0];
    chartY = _chartData.y[0] + yCalibDiff * yScale;

    Position displayPos;
    getDisplayPos(&displayPos);

    *x = chartX * _view.scale - displayPos.x;
    *y = chartY * _view.scale - displayPos.y;
}

/// <summary>
/// Convert chart position to location. Chart must be calibrated.
/// </summary>
void chartPosToLocation(int x, int y, Locn* loc)
{
    int xCalibDiff = _chartData.x[1] - _chartData.x[0];
    double xDiff = x - _chartData.x[0];
    double lonScale = xDiff / xCalibDiff;

    int yCalibDiff = _chartData.y[1] - _chartData.y[0];
    double yDiff = y - _chartData.y[0];
    double latScale = yDiff / yCalibDiff;

    double latCalibDiff = _chartData.lat[1] - _chartData.lat[0];
    double lonCalibDiff = _chartData.lon[1] - _chartData.lon[0];

    loc->lat = _chartData.lat[0] + latCalibDiff * latScale;
    loc->lon = _chartData.lon[0] + lonCalibDiff * lonScale;
}
