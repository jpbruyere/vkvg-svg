#ifndef VKVG_SVG_H
#define VKVG_SVG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vkvg.h"

//#define DEBUG_LOG

#ifdef DEBUG_LOG
#define LOG(...) fprintf (stdout, __VA_ARGS__)
#else
#define LOG
#endif

VkvgSurface vkvg_surface_create_from_svg (VkvgDevice dev, uint32_t width, uint32_t height, const char* filePath);
VkvgSurface vkvg_surface_create_from_svg_fragment (VkvgDevice dev, uint32_t width, uint32_t height, const char* svgFragment);

#ifdef __cplusplus
}
#endif

#endif // VKVG_SVG_H
