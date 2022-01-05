#ifndef VKVG_SVG_INTERNAL_H
#define VKVG_SVG_INTERNAL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include "vkengine.h"
#include <stdarg.h>
#include <ctype.h>

#include "vkvg.h"

#define ARRAY_INIT	8
#define ARRAY_ELEMENT_TYPE void*
#include "array.h"

#define DEBUG_LOG

#ifdef DEBUG_LOG
#define LOG(...) fprintf (stdout, __VA_ARGS__)
#else
#define LOG
#endif

#define degToRad(x) (x * M_PI / 180.0)

typedef enum {
	svg_element_type_rect,
	svg_element_type_circle,
	svg_element_type_line,
	svg_element_type_ellipse,
	svg_element_type_polygon,
	svg_element_type_polyline,
	svg_element_type_linear_gradient,
	svg_element_type_radial_gradient
}svg_element_type;

typedef enum {
	svg_paint_type_none,
	svg_paint_type_solid,
	svg_paint_type_pattern,
}svg_paint_type;

typedef enum {
	svg_unit_cm,
	svg_unit_mm,
	svg_unit_Q,
	svg_unit_in,
	svg_unit_pt,
	svg_unit_pc,
	svg_unit_px,
	svg_unit_percentage,
	svg_unit_deg,
	svg_unit_grad,
	svg_unit_rad
}svg_unit;

typedef enum {
	svg_gradient_unit_objectBoundingBox,
	svg_gradient_unit_userSpaceOnUse
}svg_gradient_unit;

typedef enum {
	svg_text_anchor_start,
	svg_text_anchor_middle,
	svg_text_anchor_end
}svg_text_anchor;

typedef struct {
	float number;
	svg_unit units;
} svg_length_or_percentage;

typedef struct {
	uint32_t hash;
	svg_element_type type;
	uint32_t xlinkHref;//0 if unused
} svg_element_id;

typedef struct {
	svg_element_id id;
	svg_length_or_percentage x;
	svg_length_or_percentage y;
	svg_length_or_percentage w;
	svg_length_or_percentage h;
	svg_length_or_percentage rx;
	svg_length_or_percentage ry;
}svg_element_rect;

typedef struct {
	svg_element_id id;
	svg_length_or_percentage cx;
	svg_length_or_percentage cy;
	svg_length_or_percentage r;
}svg_element_circle;

typedef struct {
	svg_element_id id;
	svg_length_or_percentage x1;
	svg_length_or_percentage y1;
	svg_length_or_percentage x2;
	svg_length_or_percentage y2;
}svg_element_line;

typedef struct {
	svg_element_id id;
	svg_length_or_percentage cx;
	svg_length_or_percentage cy;
	svg_length_or_percentage rx;
	svg_length_or_percentage ry;
}svg_element_ellipse;

typedef struct {
	svg_element_id id;
}svg_element_polygon;

typedef struct {
	svg_element_id id;
}svg_element_polyline;

typedef struct {
	svg_element_id id;
	svg_gradient_unit gradientUnits;
	svg_length_or_percentage cx;
	svg_length_or_percentage cy;
	svg_length_or_percentage fx;
	svg_length_or_percentage fy;
	svg_length_or_percentage r;
	VkvgPattern pattern;
}svg_element_radial_gradient;

typedef struct {
	svg_element_id id;
	svg_gradient_unit gradientUnits;
	svg_length_or_percentage x1;
	svg_length_or_percentage x2;
	svg_length_or_percentage y1;
	svg_length_or_percentage y2;
	VkvgPattern pattern;
}svg_element_linear_gradient;

typedef struct {
	svg_paint_type hasStroke;
	svg_paint_type hasFill;
	uint32_t stroke;
	uint32_t fill;
	float opacity;
	float fill_opacity;
	float stroke_opacity;
	svg_text_anchor text_anchor;
}svg_attributes;

uint32_t _get_element_hash (void* elt);
svg_element_type _get_element_type (void* elt);

typedef struct {
	float x;
	float y;
	float w;
	float h;
} svg_viewbox;

typedef struct {
	char		elt[128];
	char		att[1024];
	char		value[1024*1024];
	VkvgDevice	dev;
	VkvgContext	ctx;
	VkvgSurface surf;
	uint32_t	width;//force surface width & height
	uint32_t	height;
	svg_viewbox viewBox;
	bool		fit;//fit rendering surface
	bool		preserveAspectRatio;
	bool		skip;//skip tag and children
	uint32_t	currentIdHash;
	uint32_t	currentXlinkHref;
	array_t*	idList;
	/*long		currentTagStartPos;*/

} svg_context;

enum prevCmd {none, quad, cubic};

int skip_children (svg_context* svg, FILE* f, svg_attributes attribs, void* parentData);
int read_tag (svg_context* svg, FILE* f, svg_attributes attribs);

#define get_attribute fscanf(f, " %[^=>]=%*[\"']%[^\"']%*[\"']", svg->att, svg->value)

#define read_tag_end \
	svg->currentXlinkHref = 0;									\
	svg->currentIdHash = 0;										\
	if (res < 0) {												\
		LOG("error parsing: %s\n", svg->att);					\
		res -1;													\
	}else if (res == 1) {										\
		if (getc(f) != '>') {									\
			LOG("parsing error, expecting '>'\n");				\
			res = -1;											\
		} else													\
			res = 0;											\
	}else if (getc(f) != '>') {									\
		LOG("parsing error, expecting '>'\n");					\
		res -1;													\
	}else\
		res = 1;

int skip_attributes_and_children (svg_context* svg, FILE* f, svg_attributes attribs);

#define read_element_start										\
	fscanf(f, "%[^<]", svg->value);								\
	res = fscanf(f, " <%[^> \n\r]", svg->elt);					\
	if (res < 0)												\
		return 0;												\
	if (!res) {													\
		res = fscanf(f, "%*[^<]<%[^> \n\r]", svg->elt);			\
		if (!res) {												\
			LOG("element name parsing error (%s)\n", svg->elt);	\
			return -1;											\
		}														\
	}															\
	if (svg->elt[0] == '/') {									\
		if (getc(f) != '>') {									\
			LOG("parsing error, expecting '>'\n");				\
			return -1;											\
		}														\
		return 0;												\
	}															\
	if (svg->elt[0] == '!') {									\
		if (!strcmp(svg->elt,"!DOCTYPE")) {						\
			while (!feof (f))									\
				if (fgetc(f)=='>')								\
					break;										\
		} if (!strncmp(svg->elt, "!--", 3)) {					\
			while (!feof (f)) {									\
				if (fgetc(f)=='-') {							\
					if (fgetc(f)=='-' && fgetc(f)=='>')			\
						break;									\
					else										\
						fseek (f, -1, SEEK_CUR);				\
				}												\
			}													\
		}														\
		continue;												\
	}															\
	if (svg->skip) {											\
		res = skip_attributes_and_children (svg, f, attribs);	\
		continue;												\
	}															\

#define skip_element {																\
	svg->skip = true;																\
	res = skip_attributes_and_children (svg, f, attribs);							\
	svg->skip = false;																\
}

#define read_attributes_loop_start_lite \
	while (style || res > 0) {														\
		if (!style && res == 1 && svg->att[0] == '/')								\
			break;																	\
		if (!strcasecmp(svg->att, "style")) {										\
			int styleLength = strlen (svg->value);									\
			if (styleLength > 0) {													\
				style = (char*)malloc(styleLength);									\
				memccpy(style, svg->value, 0, styleLength);							\
				pfStyle = fmemopen(style, styleLength, "r");						\
				res = fscanf(pfStyle, " %[^:]:%[^;];", svg->att, svg->value);		\
				continue;															\
			}																		\
		}																			\

#define read_attributes_loop_start \
	FILE* pfStyle = NULL;															\
	char* style = NULL;																\
	int res = get_attribute;														\
	read_attributes_loop_start_lite

#define read_attributes_loop_end \
	if (style) {																	\
		if (feof(pfStyle)) {														\
			fclose (pfStyle);														\
			free (style);															\
			style = NULL;															\
		} else {																	\
			res = fscanf(pfStyle, " %[^:]:%[^;];", svg->att, svg->value);			\
			continue;																\
		}																			\
	}																				\
	res = get_attribute;															\
}

#endif // VKVG_SVG_INTERNAL_H
