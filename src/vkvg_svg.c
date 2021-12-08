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

#include "vkvg_svg.h"
#include "vkvg_extensions.h"

#ifdef DEBUG_LOG
#define LOG(msg,...) fLOG (stdout, msg, __VA_ARGS__);
#else
#define LOG
#endif

static int res;

typedef struct svg_context_t {
	char		elt[128];
	char		att[1024];
	char		value[1024*1024];
	VkvgDevice	dev;
	VkvgContext	ctx;
	VkvgSurface surf;
	uint32_t	width;//force surface width & height
	uint32_t	height;
} svg_context;

int read_tag (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill);

#define get_attribute fscanf(f, " %[^=>]=%*[\"']%[^\"']%*[\"']", svg->att, svg->value)

#define read_tag_end \
	if (res < 0) {											\
		LOG("error parsing: %s\n", svg->att);				\
		return -1;											\
	}														\
	if (res == 1) {											\
		if (getc(f) != '>') {								\
			LOG("parsing error, expecting '>'\n");			\
			return -1;										\
		}													\
		return 0;											\
	}														\
	if (getc(f) != '>') {									\
		LOG("parsing error, expecting '>'\n");			\
		return -1;											\
	}														\
	res = read_tag(svg, f, hasStroke, hasFill, stroke, fill);\



uint32_t parseColorName (const char* name) {
	int valLenght = strlen (name);

	switch(tolower(name[0])) {
	case 'a':
		switch(tolower(name[1])) {
		case 'l':
			return 0xFFFFF8F0;//AliceBlue
		case 'n':
			return 0xFFD7EBFA;//AntiqueWhite
		case 'q':
			if (!strncasecmp(&name[2],"ua",2)) {//down
				if (valLenght == 4)
					return 0xFFFFFF00;//Aqua
				if (!strcasecmp(&name[4],"marine"))
					return 0xFFD4FF7F;//Aquamarine
				else
					return 0;//UNKNOWN COLOR
			}
			break;
		case 'z':
			return 0xFFFFFFF0;//Azure
		}
		break;
	case 'b':
		switch(tolower(name[1])) {
		case 'e':
			return 0xFFDCF5F5;//Beige
		case 'i':
			return 0xFFC4E4FF;//Bisque
		case 'l':
			switch(tolower(name[2])) {
			case 'a':
				switch(tolower(name[3])) {
				case 'c':
					return 0xFF000000;//Black
				case 'n':
					return 0xFFCDEBFF;//BlanchedAlmond
				}
				break;
			case 'u':
				if (tolower(name[3]) == 'e') {//down
					if (valLenght == 4)
						return 0xFFFF0000;//Blue
					if (!strcasecmp(&name[4],"violet"))
						return 0xFFE22B8A;//BlueViolet
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
			break;
		case 'r':
			return 0xFF2A2AA5;//Brown
		case 'u':
			return 0xFF87B8DE;//Burlywood
		}
		break;
	case 'c':
		switch(tolower(name[1])) {
		case 'a':
			return 0xFFA09E5F;//CadetBlue
		case 'h':
			switch(tolower(name[2])) {
			case 'a':
				return 0xFF00FF7F;//Chartreuse
			case 'o':
				return 0xFF1E69D2;//Chocolate
			}
			break;
		case 'o':
			if (tolower(name[2]) == 'r') {//up
				switch(tolower(name[3])) {
				case 'a':
					return 0xFF507FFF;//Coral
				case 'n':
					switch(tolower(name[4])) {
					case 'f':
						return 0xFFED9564;//Cornflower
					case 's':
						return 0xFFDCF8FF;//Cornsilk
					}
					break;
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case 'r':
			return 0xFF3C14DC;//Crimson
		case 'y':
			return 0xFFFFFF00;//Cyan
		}
		break;
	case 'd':
		switch(tolower(name[1])) {
		case 'a':
			if (!strncasecmp(&name[2],"rk",2)) {//up
				switch(tolower(name[4])) {
				case 'b':
					return 0xFF8B0000;//DarkBlue
				case 'c':
					return 0xFF8B8B00;//DarkCyan
				case 'g':
					switch(tolower(name[5])) {
					case 'o':
						return 0xFF0B86B8;//DarkGoldenrod
					case 'r':
						switch(tolower(name[6])) {
						case 'a':
							return 0xFFA9A9A9;//DarkGray
						case 'e':
							return 0xFF006400;//DarkGreen
						}
						break;
					}
					break;
				case 'k':
					return 0xFF6BB7BD;//DarkKhaki
				case 'm':
					return 0xFF8B008B;//DarkMagenta
				case 'o':
					switch(tolower(name[5])) {
					case 'l':
						return 0xFF2F6B55;//DarkOliveGreen
					case 'r':
						switch(tolower(name[6])) {
						case 'a':
							return 0xFF008CFF;//DarkOrange
						case 'c':
							return 0xFFCC3299;//DarkOrchid
						}
						break;
					}
					break;
				case 'r':
					return 0xFF00008B;//DarkRed
				case 's':
					switch(tolower(name[5])) {
					case 'a':
						return 0xFF7A96E9;//DarkSalmon
					case 'e':
						return 0xFF8FBC8F;//DarkSeaGreen
					case 'l':
						if (!strncasecmp(&name[6],"ate",3)) {//up
							switch(tolower(name[9])) {
							case 'b':
								return 0xFF8B3D48;//DarkSlateBlue
							case 'g':
								return 0xFF4F4F2F;//DarkSlateGray
							}
						} else
							return 0;//UNKNOWN COLOR
						break;
					}
					break;
				case 't':
					return 0xFFD1CE00;//DarkTurquoise
				case 'v':
					return 0xFFD30094;//DarkViolet
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case 'e':
			if (!strncasecmp(&name[2],"ep",2)) {//up
				switch(tolower(name[4])) {
				case 'p':
					return 0xFF9314FF;//DeepPink
				case 's':
					return 0xFFFFBF00;//DeepSkyBlue
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case 'i':
			return 0xFF696969;//DimGray
		case 'o':
			return 0xFFFF901E;//DodgerBlue
		}
		break;
	case 'f':
		switch(tolower(name[1])) {
		case 'i':
			return 0xFF2222B2;//Firebrick
		case 'l':
			return 0xFFF0FAFF;//FloralWhite
		case 'o':
			return 0xFF228B22;//ForestGreen
		case 'u':
			return 0xFFFF00FF;//Fuchsia
		}
		break;
	case 'g':
		switch(tolower(name[1])) {
		case 'a':
			return 0xFFDCDCDC;//Gainsboro
		case 'h':
			return 0xFFFFF8F8;//GhostWhite
		case 'o':
			if (!strncasecmp(&name[2],"ld",2)) {//down
				if (valLenght == 4)
					return 0xFF00D7FF;//Gold
				if (!strcasecmp(&name[4],"enrod"))
					return 0xFF20A5DA;//Goldenrod
				else
					return 0;//UNKNOWN COLOR
			}
			break;
		case 'r':
			switch(tolower(name[2])) {
			case 'a':
				return 0xFFBEBEBE;//Gray
			case 'e':
				if (!strncasecmp(&name[3],"en",2)) {//down
					if (valLenght == 5)
						return 0xFF00FF00;//Green
					if (!strcasecmp(&name[5],"yellow"))
						return 0xFF2FFFAD;//GreenYellow
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
			break;
		}
		break;
	case 'h':
		if (tolower(name[1]) == 'o') {//up
			switch(tolower(name[2])) {
			case 'n':
				return 0xFFF0FFF0;//Honeydew
			case 't':
				return 0xFFB469FF;//HotPink
			}
		} else
			return 0;//UNKNOWN COLOR
		break;
	case 'i':
		switch(tolower(name[1])) {
		case 'n':
			if (!strncasecmp(&name[2],"di",2)) {//up
				switch(tolower(name[4])) {
				case 'a':
					return 0xFF5C5CCD;//IndianRed
				case 'g':
					return 0xFF82004B;//Indigo
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case 'v':
			return 0xFFF0FFFF;//Ivory
		}
		break;
	case 'k':
		return 0xFF8CE6F0;//Khaki
	case 'l':
		switch(tolower(name[1])) {
		case 'a':
			switch(tolower(name[2])) {
			case 'v':
				if (!strncasecmp(&name[3],"ender",5)) {//down
					if (valLenght == 8)
						return 0xFFFAE6E6;//Lavender
					if (!strcasecmp(&name[8],"blush"))
						return 0xFFF5F0FF;//LavenderBlush
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			case 'w':
				return 0xFF00FC7C;//LawnGreen
			}
			break;
		case 'e':
			return 0xFFCDFAFF;//LemonChiffon
		case 'i':
			switch(tolower(name[2])) {
			case 'g':
				if (!strncasecmp(&name[3],"ht",2)) {//up
					switch(tolower(name[5])) {
					case 'b':
						return 0xFFE6D8AD;//LightBlue
					case 'c':
						switch(tolower(name[6])) {
						case 'o':
							return 0xFF8080F0;//LightCoral
						case 'y':
							return 0xFFFFFFE0;//LightCyan
						}
						break;
					case 'g':
						switch(tolower(name[6])) {
						case 'o':
							return 0xFFD2FAFA;//LightGoldenrod
						case 'r':
							switch(tolower(name[7])) {
							case 'a':
								return 0xFFD3D3D3;//LightGray
							case 'e':
								return 0xFF90EE90;//LightGreen
							}
							break;
						}
						break;
					case 'p':
						return 0xFFC1B6FF;//LightPink
					case 's':
						switch(tolower(name[6])) {
						case 'a':
							return 0xFF7AA0FF;//LightSalmon
						case 'e':
							return 0xFFAAB220;//LightSeaGreen
						case 'k':
							return 0xFFFACE87;//LightSkyBlue
						case 'l':
							return 0xFF998877;//LightSlateGray
						case 't':
							return 0xFFDEC4B0;//LightSteelBlue
						}
						break;
					case 'y':
						return 0xFFE0FFFF;//LightYellow
					}
				} else
					return 0;//UNKNOWN COLOR
				break;
			case 'm':
				if (tolower(name[3]) == 'e') {//down
					if (valLenght == 4)
						return 0xFF00FF00;//Lime
					if (!strcasecmp(&name[4],"green"))
						return 0xFF32CD32;//LimeGreen
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			case 'n':
				return 0xFFE6F0FA;//Linen
			}
			break;
		}
		break;
	case 'm':
		switch(tolower(name[1])) {
		case 'a':
			switch(tolower(name[2])) {
			case 'g':
				return 0xFFFF00FF;//Magenta
			case 'r':
				return 0xFF6030B0;//Maroon
			}
			break;
		case 'e':
			if (!strncasecmp(&name[2],"dium",4)) {//up
				switch(tolower(name[6])) {
				case 'a':
					return 0xFFAACD66;//MediumAquamarine
				case 'b':
					return 0xFFCD0000;//MediumBlue
				case 'o':
					return 0xFFD355BA;//MediumOrchid
				case 'p':
					return 0xFFDB7093;//MediumPurple
				case 's':
					switch(tolower(name[7])) {
					case 'e':
						return 0xFF71B33C;//MediumSeaGreen
					case 'l':
						return 0xFFEE687B;//MediumSlateBlue
					case 'p':
						return 0xFF9AFA00;//MediumSpringGreen
					}
					break;
				case 't':
					return 0xFFCCD148;//MediumTurquoise
				case 'v':
					return 0xFF8515C7;//MediumVioletRed
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case 'i':
			switch(tolower(name[2])) {
			case 'd':
				return 0xFF701919;//MidnightBlue
			case 'n':
				return 0xFFFAFFF5;//MintCream
			case 's':
				return 0xFFE1E4FF;//MistyRose
			}
			break;
		case 'o':
			return 0xFFB5E4FF;//Moccasin
		}
		break;
	case 'n':
		if (!strncasecmp(&name[1],"av",2)) {//up
			switch(tolower(name[3])) {
			case 'a':
				return 0xFFADDEFF;//NavajoWhite
			case 'y':
				return 0xFF800000;//Navy
			}
		} else
			return 0;//UNKNOWN COLOR
		break;
	case 'o':
		switch(tolower(name[1])) {
		case 'l':
			switch(tolower(name[2])) {
			case 'd':
				return 0xFFE6F5FD;//OldLace
			case 'i':
				if (!strncasecmp(&name[3],"ve",2)) {//down
					if (valLenght == 5)
						return 0xFF008080;//Olive
					if (!strcasecmp(&name[5],"drab"))
						return 0xFF238E6B;//OliveDrab
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
			break;
		case 'r':
			switch(tolower(name[2])) {
			case 'a':
				if (!strncasecmp(&name[3],"nge",3)) {//down
					if (valLenght == 6)
						return 0xFF00A5FF;//Orange
					if (!strcasecmp(&name[6],"red"))
						return 0xFF0045FF;//OrangeRed
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			case 'c':
				return 0xFFD670DA;//Orchid
			}
			break;
		}
		break;
	case 'p':
		switch(tolower(name[1])) {
		case 'a':
			switch(tolower(name[2])) {
			case 'l':
				if (tolower(name[3]) == 'e') {//up
					switch(tolower(name[4])) {
					case 'g':
						switch(tolower(name[5])) {
						case 'o':
							return 0xFFAAE8EE;//PaleGoldenrod
						case 'r':
							return 0xFF98FB98;//PaleGreen
						}
						break;
					case 't':
						return 0xFFEEEEAF;//PaleTurquoise
					case 'v':
						return 0xFF9370DB;//PaleVioletRed
					}
				} else
					return 0;//UNKNOWN COLOR
				break;
			case 'p':
				return 0xFFD5EFFF;//PapayaWhip
			}
			break;
		case 'e':
			switch(tolower(name[2])) {
			case 'a':
				return 0xFFB9DAFF;//PeachPuff
			case 'r':
				return 0xFF3F85CD;//Peru
			}
			break;
		case 'i':
			return 0xFFCBC0FF;//Pink
		case 'l':
			return 0xFFDDA0DD;//Plum
		case 'o':
			return 0xFFE6E0B0;//PowderBlue
		case 'u':
			return 0xFFF020A0;//Purple
		}
		break;
	case 'r':
		switch(tolower(name[1])) {
		case 'e':
			return 0xFF0000FF;//Red
		case 'o':
			switch(tolower(name[2])) {
			case 's':
				return 0xFF8F8FBC;//RosyBrown
			case 'y':
				return 0xFFE16941;//RoyalBlue
			}
			break;
		}
		break;
	case 's':
		switch(tolower(name[1])) {
		case 'a':
			switch(tolower(name[2])) {
			case 'd':
				return 0xFF13458B;//SaddleBrown
			case 'l':
				return 0xFF7280FA;//Salmon
			case 'n':
				return 0xFF60A4F4;//SandyBrown
			}
			break;
		case 'e':
			if (tolower(name[2]) == 'a') {//up
				switch(tolower(name[3])) {
				case 'g':
					return 0xFF578B2E;//SeaGreen
				case 's':
					return 0xFFEEF5FF;//Seashell
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case 'i':
			switch(tolower(name[2])) {
			case 'e':
				return 0xFF2D52A0;//Sienna
			case 'l':
				return 0xFFC0C0C0;//Silver
			}
			break;
		case 'k':
			return 0xFFEBCE87;//SkyBlue
		case 'l':
			if (!strncasecmp(&name[2],"ate",3)) {//up
				switch(tolower(name[5])) {
				case 'b':
					return 0xFFCD5A6A;//SlateBlue
				case 'g':
					return 0xFF908070;//SlateGray
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case 'n':
			return 0xFFFAFAFF;//Snow
		case 'p':
			return 0xFF7FFF00;//SpringGreen
		case 't':
			return 0xFFB48246;//SteelBlue
		}
		break;
	case 't':
		switch(tolower(name[1])) {
		case 'a':
			return 0xFF8CB4D2;//Tan
		case 'e':
			return 0xFF808000;//Teal
		case 'h':
			return 0xFFD8BFD8;//Thistle
		case 'o':
			return 0xFF4763FF;//Tomato
		case 'u':
			return 0xFFD0E040;//Turquoise
		}
		break;
	case 'v':
		return 0xFFEE82EE;//Violet
	case 'w':
		if (tolower(name[1]) == 'h') {//up
			switch(tolower(name[2])) {
			case 'e':
				return 0xFFB3DEF5;//Wheat
			case 'i':
				if (!strncasecmp(&name[3],"te",2)) {//down
					if (valLenght == 5)
						return 0xFFFFFFFF;//White
					if (!strcasecmp(&name[5],"smoke"))
						return 0xFFF5F5F5;//WhiteSmoke
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
		} else
			return 0;//UNKNOWN COLOR
		break;
	case 'y':
		if (!strncasecmp(&name[1],"ellow",5)) {//down
			if (valLenght == 6)
				return 0xFF00FFFF;//Yellow
			if (!strcasecmp(&name[6],"green"))
				return 0xFF32CD9A;//YellowGreen
			else
				return 0;//UNKNOWN COLOR
		}
		break;
	}
}
bool parse_floats (FILE* f, int floatCount, ...) {
	va_list args;
	va_start(args, floatCount);

	for (int i=0; i<floatCount; i++) {
		float* pF = va_arg(args, float*);
		fscanf(f, ",");
		if (fscanf(f, " %f ", pF)!=1) {
			va_end(args);
			return false;
		}
	}
	va_end(args);
	return true;
}
float parse_lenghtOrPercentage (const char* measure) {
	float m = 0;
	res = sscanf(measure, "%f", &m);
	return m;
}
bool parse_color (const char* colorString, bool* isEnabled, uint32_t* colorValue) {

	if (colorString[0] == '#') {
		char color[7];
		res = sscanf(&colorString[1], " %[0-9A-Fa-f]", color);
		char hexColorString[12];
		if (strlen(color) == 3)
			sprintf(hexColorString, "0xff%c%c%c%c%c%c", color[2],color[2],color[1],color[1],color[0],color[0]);
		else
			sprintf(hexColorString, "0xff%c%c%c%c%c%c", color[4],color[5],color[2],color[3],color[0],color[1]);
		sscanf(hexColorString, "%x", colorValue);
		*isEnabled = true;
		return true;
	}
	uint32_t r = 0, g = 0, b = 0, a = 0;
	if (sscanf(colorString, "rgb ( %d%*[ ,]%d%*[ ,]%d )", &r, &g, &b))
		*colorValue = 0xff000000 + (b << 16) + (g << 8) + r;
	else if (sscanf(colorString, "rgba ( %d%*[ ,]%d%*[ ,]%d%*[ ,]%d )", &r, &g, &b, &a))
		*colorValue = (a << 24) + (b << 16) + (g << 8) + r;
	else if (!strcmp (colorString, "none"))
		*colorValue = 0;
	else if (strcasecmp (colorString, "currentColor"))
		*colorValue = parseColorName(colorString);
	*isEnabled = colorValue > 0;
	return true;
}

int draw (svg_context* svg, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	if (hasFill) {
		vkvg_set_source_color(svg->ctx, fill);
		if (hasStroke) {
			vkvg_fill_preserve(svg->ctx);
			vkvg_set_source_color(svg->ctx, stroke);
			vkvg_stroke(svg->ctx);
		} else
			vkvg_fill (svg->ctx);
	} else if (hasStroke) {
		vkvg_set_source_color(svg->ctx, stroke);
		vkvg_stroke(svg->ctx);
	}
}

int read_common_attributes (svg_context* svg, bool *hasStroke, bool *hasFill, uint32_t *stroke, uint32_t *fill) {
	if (!strcmp(svg->att, "fill"))
		parse_color (svg->value, hasFill, fill);
	else if (!strcmp(svg->att, "fill-rule")) {
		if (!strcmp(svg->value, "evenodd"))
			vkvg_set_fill_rule(svg->ctx, VKVG_FILL_RULE_EVEN_ODD);
		else
			vkvg_set_fill_rule(svg->ctx, VKVG_FILL_RULE_NON_ZERO);
	}else if (!strcmp(svg->att, "stroke"))
		parse_color (svg->value, hasStroke, stroke);
	else if (!strcmp (svg->att, "stroke-width"))
		vkvg_set_line_width(svg->ctx, parse_lenghtOrPercentage (svg->value));
	else if (!strcmp (svg->att, "stroke-linecap")) {
		if (!strcasecmp(svg->value, "round"))
			vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_ROUND);
		else if (!strcasecmp(svg->value, "square"))
			vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_SQUARE);
		else if (!strcasecmp(svg->value, "butt"))
			vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_BUTT);
	} else if (!strcmp (svg->att, "stroke-linejoin")) {
		if (!strcasecmp(svg->value, "round"))
			vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_ROUND);
		else if (!strcasecmp(svg->value, "bevel"))
			vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_BEVEL);
		else if (!strcasecmp(svg->value, "miter"))
			vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_MITER);
	} else if (!strcmp (svg->att, "transform")) {
		FILE *tmp = fmemopen(svg->value, strlen (svg->value), "r");
		char transform[16];
		while (!feof(tmp)) {
			if (fscanf(tmp, " %[^(](", transform) == 1) {
				if (!strcmp (transform, "matrix")) {
					vkvg_matrix_t m, current, newMat;
					if (!parse_floats(tmp, 6, &m.xx, &m.yx, &m.xy, &m.yy, &m.x0, &m.y0)) {
						LOG ("error parsing transformation matrix\n");
						break;
					}
					//vkvg_matrix_init_identity(&m);
					vkvg_get_matrix(svg->ctx, &current);
					vkvg_matrix_multiply(&newMat, &m, &current);
					vkvg_set_matrix(svg->ctx, &newMat);
				}
				char c = getc(tmp);
				if (c!=')') {
					LOG ("error parsing transformation, expecting ')', having %c\n", c);
					break;
				}
			} else
				break;
		}

		fclose (tmp);

	} else
		return 0;
	return 1;
}

int read_rect_attributes (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	int x = 0, y = 0, w = 0, h = 0, rx = 0, ry = 0;
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(svg, &hasStroke, &hasFill, &stroke, &fill)) {
			if (!strcmp (svg->att, "x"))
				x = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "y"))
				y = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "width"))
				w = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "height"))
				h = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "rx"))
				rx = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "ry"))
				ry = parse_lenghtOrPercentage(svg->value);
			else
				LOG("Unprocessed attribute: %s->%s\n", svg->elt, svg->att);
		}
		res = get_attribute;
	}
	if (w && h && (hasFill || hasStroke)) {
		vkvg_rectangle(svg->ctx, x, y, w, h);
		draw (svg, hasStroke, hasFill, stroke, fill);
	}
	read_tag_end
	return res;
}
int read_line_attributes (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	int x1 = 0, y1 = 0, x2 = 0, y2;
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(svg, &hasStroke, &hasFill, &stroke, &fill)) {
			if (!strcmp (svg->att, "x1"))
				x1 = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "y1"))
				y1 = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "x2"))
				x2 = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "y2"))
				y2 = parse_lenghtOrPercentage(svg->value);
			else
				LOG("Unprocessed attribute: %s->%s\n", svg->elt, svg->att);
		}
		res = get_attribute;
	}
	if (hasStroke) {
		vkvg_move_to(svg->ctx, x1, y1);
		vkvg_line_to(svg->ctx, x2, y2);
		draw (svg, hasStroke, false, stroke, 0);
	}
	read_tag_end
	return res;
}
int read_circle_attributes (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	int cx = 0, cy = 0, r;
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(svg, &hasStroke, &hasFill, &stroke, &fill)) {
			if (!strcmp (svg->att, "cx"))
				cx = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "cy"))
				cy = parse_lenghtOrPercentage(svg->value);
			else if (!strcmp (svg->att, "r"))
				r = parse_lenghtOrPercentage(svg->value);
			else
				LOG("Unprocessed attribute: %s->%s\n", svg->elt, svg->att);
		}
		res = get_attribute;
	}
	if (r > 0 && (hasFill || hasStroke)) {
		vkvg_arc(svg->ctx, cx, cy, r, 0, M_PI * 2);
		draw (svg, hasStroke, hasFill, stroke, fill);
	}
	read_tag_end
	return res;
}
int read_polyline_attributes (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	float x = 0, y = 0;
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(svg, &hasStroke, &hasFill, &stroke, &fill)) {
			if (!strcasecmp (svg->att, "points")) {
				FILE *tmp = fmemopen(svg->value, strlen (svg->value), "r");
				if (parse_floats(tmp, 2, &x, &y)) {
					vkvg_move_to(svg->ctx, x, y);

					while (!feof(tmp)) {
						if (!parse_floats(tmp, 2, &x, &y))
							break;
						vkvg_line_to(svg->ctx, x, y);
					}
				}
				fclose (tmp);
			} else
				LOG("Unprocessed attribute: %s->%s\n", svg->elt, svg->att);
		}
		res = get_attribute;
	}
	if (hasFill || hasStroke)
		draw (svg, hasStroke, hasFill, stroke, fill);
	read_tag_end
	return res;
}
enum prevCmd {none, quad, cubic};
int read_path_attributes (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(svg, &hasStroke, &hasFill, &stroke, &fill)) {
			float x, y, c1x, c1y, c2x, c2y, cpX, cpY, rx, ry, rotx, large, sweep;
			enum prevCmd prev = none;
			int repeat=0;
			char c;
			if (!strcmp (svg->att, "d")) {
				FILE *tmp = fmemopen(svg->value, strlen (svg->value), "r");
				bool parseError = false, result = false;
				while (!(parseError || feof(tmp))) {
					if (!repeat && fscanf(tmp, " %c ", &c) != 1) {
						LOG ("error parsing path: expectin char\n");
						break;
					}

					switch (c) {
					case 'M':
						if (parse_floats(tmp, 2, &x, &y))
							vkvg_move_to (svg->ctx, x, y);
						else
							parseError = true;
						break;
					case 'm':
						if (parse_floats(tmp, 2, &x, &y))
							vkvg_rel_move_to (svg->ctx, x, y);
						else
							parseError = true;
						break;
					case 'L':
						if (repeat)
							result = parse_floats(tmp, 1, &y);
						else
							result = parse_floats(tmp, 2, &x, &y);
						if (result) {
							vkvg_line_to (svg->ctx, x, y);
							repeat = parse_floats(tmp, 1, &x);
						} else
							parseError = true;
						break;
					case 'l':
						if (repeat)
							result = parse_floats(tmp, 1, &y);
						else
							result = parse_floats(tmp, 2, &x, &y);
						if (result) {
							vkvg_rel_line_to (svg->ctx, x, y);
							repeat = parse_floats(tmp, 1, &x);
						} else
							parseError = true;
						break;
					case 'H':
						if (parse_floats(tmp, 1, &x)) {
							vkvg_get_current_point (svg->ctx, &c1x, &c1y);
							vkvg_line_to (svg->ctx, x, c1y);
						} else
							parseError = true;
						break;
					case 'h':
						if (parse_floats(tmp, 1, &x))
							vkvg_rel_line_to (svg->ctx, x, 0);
						else
							parseError = true;
						break;
					case 'V':
						if (parse_floats(tmp, 1, &y)) {
							vkvg_get_current_point (svg->ctx, &c1x, &c1y);
							vkvg_line_to (svg->ctx, c1x, y);
						} else
							parseError = true;
						break;
					case 'v':
						if (parse_floats(tmp, 1, &y))
							vkvg_rel_line_to (svg->ctx, 0, y);
						else
							parseError = true;
						break;
					case 'Q':
						if (repeat)
							result = parse_floats(tmp, 3, &c1y, &x, &y);
						else
							result = parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
						if (result) {
							vkvg_quadratic_to (svg->ctx, c1x, c1y, x, y);
							prev = quad;
							repeat = parse_floats(tmp, 1, &c1x);
							continue;
						} else
							parseError = true;
						break;
					case 'q':
						if (repeat)
							result = parse_floats(tmp, 3, &c1y, &x, &y);
						else
							result = parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
						if (result) {
							vkvg_get_current_point(svg->ctx, &cpX, &cpY);
							vkvg_rel_quadratic_to (svg->ctx, c1x, c1y, x, y);
							prev = quad;
							c1x += cpX;
							c1y += cpY;
							repeat = parse_floats(tmp, 1, &c1x);
							continue;
						} else
							parseError = true;
						break;
					case 'T':
						if (repeat)
							result = parse_floats(tmp, 1, &y);
						else
							result = parse_floats(tmp, 2, &x, &y);
						if (result) {
							vkvg_get_current_point(svg->ctx, &cpX, &cpY);
							if (prev == quad) {
								c1x = 2.0 * cpX - c1x;
								c1y = 2.0 * cpY - c1y;
							} else {
								c1x = x;
								c1y = y;
							}
							vkvg_quadratic_to (svg->ctx, c1x, c1y, x, y);
							prev = quad;
							repeat = parse_floats(tmp, 1, &x);
							continue;
						} else
							parseError = true;
						break;
					case 't':
						if (repeat)
							result = parse_floats(tmp, 1, &y);
						else
							result = parse_floats(tmp, 2, &x, &y);
						if (result) {
							vkvg_get_current_point(svg->ctx, &cpX, &cpY);
							if (prev == quad) {
								c1x = (cpX-c1x);
								c1y = (cpY-c1y);
							} else {
								c1x = x;
								c1y = y;
							}
							vkvg_rel_quadratic_to (svg->ctx, c1x, c1y, x, y);
							prev = quad;
							c1x += cpX;
							c1y += cpY;
							repeat = parse_floats(tmp, 1, &x);
							continue;
						} else
							parseError = true;
						break;
					case 'C':
						if (repeat)
							result = parse_floats(tmp, 5, &c1y, &c2x, &c2y, &x, &y);
						else
							result = parse_floats(tmp, 6, &c1x, &c1y, &c2x, &c2y, &x, &y);
						if (result) {
							vkvg_curve_to (svg->ctx, c1x, c1y, c2x, c2y, x, y);
							prev = cubic;
							repeat = parse_floats(tmp, 1, &c1x);
							continue;
						} else
							parseError = true;
						break;
					case 'c':
						if (repeat)
							result = parse_floats(tmp, 5, &c1y, &c2x, &c2y, &x, &y);
						else
							result = parse_floats(tmp, 6, &c1x, &c1y, &c2x, &c2y, &x, &y);
						if (result) {
							vkvg_get_current_point(svg->ctx, &cpX, &cpY);
							vkvg_rel_curve_to (svg->ctx, c1x, c1y, c2x, c2y, x, y);
							c2x += cpX;
							c2y += cpY;
							prev = cubic;
							repeat = parse_floats(tmp, 1, &c1x);
							continue;
						} else
							parseError = true;
						break;
					case 'S':
						if (repeat)
							result = parse_floats(tmp, 3, &c1y, &x, &y);
						else
							result = parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
						if (result) {
							vkvg_get_current_point(svg->ctx, &cpX, &cpY);
							if (prev == cubic) {
								vkvg_curve_to (svg->ctx, 2.0 * cpX - c2x, 2.0 * cpY - c2y, c1x, c1y, x, y);
							} else
								vkvg_curve_to (svg->ctx, cpX, cpY, c1x, c1y, x, y);

							c2x = c1x;
							c2y = c1y;
							prev = cubic;
							repeat = parse_floats(tmp, 1, &c1x);
							continue;
						} else
							parseError = true;
						break;
					case 's':
						if (repeat)
							result = parse_floats(tmp, 3, &c1y, &x, &y);
						else
							result = parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
						if (result) {
							vkvg_get_current_point(svg->ctx, &cpX, &cpY);
							if (prev == cubic) {
								vkvg_rel_curve_to (svg->ctx, cpX - c2x, cpY - c2y, c1x, c1y, x, y);
							} else
								vkvg_rel_curve_to (svg->ctx, 0, 0, c1x, c1y, x, y);

							c2x = cpX + c1x;
							c2y = cpY + c1y;
							prev = cubic;
							repeat = parse_floats(tmp, 1, &c1x);
							continue;
						} else
							parseError = true;
						break;
					case 'A'://rx ry x-axis-rotation large-arc-flag sweep-flag x y
						if (repeat)
							result = parse_floats(tmp, 6, &ry, &rotx, &large, &sweep, &x, &y);
						else
							result = parse_floats(tmp, 7, &rx, &ry, &rotx, &large, &sweep, &x, &y);
						if (result) {
							vkvg_get_current_point(svg->ctx, &cpX, &cpY);
							bool lf = large > __FLT_EPSILON__;
							bool sw = sweep > __FLT_EPSILON__;
							vkvg_elliptic_arc(svg->ctx, cpX, cpY, x, y, lf, sw, rx, ry, rotx);
							repeat = parse_floats(tmp, 1, &rx);

						} else
							parseError = true;
						break;
					case 'a'://rx ry x-axis-rotation large-arc-flag sweep-flag x y
						if (repeat)
							result = parse_floats(tmp, 6, &ry, &rotx, &large, &sweep, &x, &y);
						else
							result = parse_floats(tmp, 7, &rx, &ry, &rotx, &large, &sweep, &x, &y);
						if (result) {
							vkvg_get_current_point(svg->ctx, &cpX, &cpY);
							bool lf = large > __FLT_EPSILON__;
							bool sw = sweep > __FLT_EPSILON__;
							vkvg_elliptic_arc(svg->ctx, cpX, cpY, cpX + x, cpY + y, lf, sw, rx, ry, rotx);
							repeat = parse_floats(tmp, 1, &rx);

						} else
							parseError = true;
						break;
					case 'z':
					case 'Z':
						vkvg_close_path(svg->ctx);
						break;
					}
					prev = none;
				}
				fclose (tmp);
			} else
				LOG("Unprocessed attribute: %s->%s\n", svg->elt, svg->att);
		}
		res = get_attribute;
	}
	if (hasFill || hasStroke)
		draw (svg, hasStroke, hasFill, stroke, fill);
	//_highlight (svg->ctx);
	read_tag_end
	return res;
}

int read_g_attributes (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(svg, &hasStroke, &hasFill, &stroke, &fill)) {
			LOG("Unprocessed attribute: %s->%s\n", svg->elt, svg->att);
		}
		res = get_attribute;
	}
	read_tag_end
	return res;
}
int read_svg_attributes (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	bool hasViewBox = false;
	int x, y, w, h, width = 0, height = 0;
	int startingPos = ftell(f);
	res = get_attribute;
	while (res == 2) {
		if (!strcmp(svg->att, "viewBox")) {
			res = sscanf(svg->value, " %d%*1[ ,]%d%*1[ ,]%d%*1[ ,]%d", &x, &y, &w, &h);
			if (res!=4)
				return -1;
			hasViewBox = true;
		} else if (!strcmp(svg->att, "width"))
			width = parse_lenghtOrPercentage (svg->value);
		else if (!strcmp(svg->att, "height"))
			height = parse_lenghtOrPercentage (svg->value);
		res = get_attribute;
	}
	int surfW = 0, surfH = 0;
	float xScale = 1, yScale = 1;
	if (svg->width)
		surfW = svg->width;
	else
		surfW = width;

	if (svg->height)
		surfH = svg->height;
	else
		surfH = height;

	//TODO viewbox x,y
	if (hasViewBox) {
		if (surfW)
			xScale = (float)surfW / w;
		else
			surfW = w;
		if (surfH)
			yScale = (float)surfH / h;
		else
			surfH = h;
	}

	svg->surf = vkvg_surface_create(svg->dev, surfW, surfH);
	svg->ctx = vkvg_create (svg->surf);

	vkvg_set_fill_rule(svg->ctx, VKVG_FILL_RULE_EVEN_ODD);
	if (xScale < yScale)
		vkvg_scale(svg->ctx, xScale, xScale);
	else
		vkvg_scale(svg->ctx, yScale, yScale);

	vkvg_clear(svg->ctx);

	fseek (f, startingPos, SEEK_SET);
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(svg, &hasStroke, &hasFill, &stroke, &fill) &&
																	strcmp(svg->att, "viewBox") &&
																	strcmp(svg->att, "width") &&
																	strcmp(svg->att, "height"))
			LOG("Unprocessed attribute: %s->%s\n", svg->elt, svg->att);
		res = get_attribute;
	}

	read_tag_end
	vkvg_destroy(svg->ctx);
	return res;
}
int read_attributes (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(svg, &hasStroke, &hasFill, &stroke, &fill)) {
			LOG("Unprocessed attribute: %s->%s\n", svg->elt, svg->att);
		}
		res = get_attribute;
	}
	read_tag_end
}
int read_tag (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {

	while (!feof (f)) {
		res = fscanf(f, " <%[^> ]", svg->elt);
		if (res < 0)
			return 0;
		if (!res) {
			res = fscanf(f, "%*[^<]<%[^> ]", svg->elt);
			if (!res) {
				LOG("element name parsing error (%s)\n", svg->elt);
				return -1;
			}
		}
		if (svg->elt[0] == '/') {
			if (getc(f) != '>') {
				LOG("parsing error, expecting '>'\n");
				return -1;
			}
			//read_attributes_ptr = NULL;
			return 0;
		}
		if (svg->elt[0] == '!') {
			if (!strcmp(svg->elt,"!DOCTYPE")) {
				while (!feof (f))
					if (fgetc(f)=='>')
						break;;
			} if (!strncmp(svg->elt, "!--", 3)) {
				while (!feof (f)) {
					if (fgetc(f)=='-') {
						if (fgetc(f)=='-' && fgetc(f)=='>')
							break;
						else
							fseek (f, -1, SEEK_CUR);
					}
				}
			}
			continue;
		}
		if (!strcmp(svg->elt,"svg"))
			res = read_svg_attributes	(svg, f, hasStroke, hasFill, stroke, fill);
		else if (!strcmp(svg->elt,"g")) {
			vkvg_save (svg->ctx);
			res = read_g_attributes		(svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!strcmp(svg->elt,"rect")) {
			vkvg_save (svg->ctx);
			res = read_rect_attributes	(svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!strcmp(svg->elt,"line")) {
			vkvg_save (svg->ctx);
			res = read_line_attributes	(svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!strcmp(svg->elt,"circle")) {
			vkvg_save (svg->ctx);
			res = read_circle_attributes (svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!strcmp(svg->elt,"polyline")) {
			vkvg_save (svg->ctx);
			res = read_polyline_attributes (svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!strcmp(svg->elt,"path")) {
			vkvg_save (svg->ctx);
			res = read_path_attributes	(svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else
			res = read_attributes		(svg, f, hasStroke, hasFill, stroke, fill);
	}
	return res;
}

VkvgSurface parse_svg_file (VkvgDevice dev, const char* filename, uint32_t width, uint32_t height) {
	FILE* f = fopen(filename, "r");
	if (f == NULL){
		perror ("vkvg_svg: file not found");
		return NULL;
	}

	svg_context svg = {0};
	svg.dev		= dev;
	svg.width	= width;
	svg.height	= height;

	read_tag (&svg, f, false, true, 0xffffffff, 0xffffffff);

	fclose(f);
	return svg.surf;
}
