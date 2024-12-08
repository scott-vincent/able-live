#pragma once
#include "able_live.h"

struct AircraftPosition {
    int x;
    int y;
    int labelX;
    int labelY;
    char text[256];
};

struct Intersect {
    int val;
    double dist;
};

void displayToChartPos(int x, int y, Position* pos);
void chartToDisplayPos(int x, int y, Position* pos);
void locationToChartPos(Locn* loc, Position* pos);
void locationToDisplay(Locn* loc, double* x, double* y);
void chartPosToLocation(int x, int y, Locn* loc);
