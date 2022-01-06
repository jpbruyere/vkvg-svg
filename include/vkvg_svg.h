#ifndef VKVG_SVG_H
#define VKVG_SVG_H

#include "vkvg.h"

//#define DEBUG_LOG

#ifdef DEBUG_LOG
#define LOG(...) fprintf (stdout, __VA_ARGS__)
#else
#define LOG
#endif

VkvgSurface parse_svg_file (VkvgDevice dev, const char* filename, uint32_t width, uint32_t height);

#endif // VKVG_SVG_H
