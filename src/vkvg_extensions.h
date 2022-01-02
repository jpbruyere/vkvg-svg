#ifndef VKVG_EXTENSIONS_H
#define VKVG_EXTENSIONS_H

#include "vkvg.h"

void vkvg_elliptic_arc (VkvgContext ctx, float x1, float y1, float x2, float y2, bool largeArc, bool counterClockWise, float rx, float ry, float phi);
void vkvg_ellipse (VkvgContext ctx, float radiusX, float radiusY, float x, float y, float rotationAngle);
void vkvg_rounded_rectangle2 (VkvgContext ctx, float x, float y, float w, float h, float rx, float ry);
void _highlight (VkvgContext ctx);
#endif // VKVG_EXTENSIONS_H
