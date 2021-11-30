#ifndef VKVG_SVG_H
#define VKVG_SVG_H

#include "vkvg.h"

VkvgSurface parse_svg_file (VkvgDevice dev, const char* filename, uint32_t width, uint32_t height);

#endif // VKVG_SVG_H
