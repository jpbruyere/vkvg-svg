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

VkvgSurface vkvg_create_surface_from_svg (VkvgDevice dev, const char* filename, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#endif // VKVG_SVG_H
