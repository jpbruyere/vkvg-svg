#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include <locale.h>
#include <wchar.h>

#include "vkvg_svg.h"
#include "vkvg_extensions.h"

#ifdef DEBUG_LOG
#define LOG(msg,...) fLOG (stdout, msg, __VA_ARGS__);
#else
#define LOG
#endif

static int res;

typedef struct svg_context_t {
	wint_t		elt[128];
	wint_t		att[1024];
	wint_t		value[1024*1024];
	VkvgDevice	dev;
	VkvgContext	ctx;
	VkvgSurface surf;
	uint32_t	width;//force surface width & height
	uint32_t	height;
} svg_context;

int read_tag (svg_context* svg, FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill);

#define get_attribute fwscanf(f, L" %[^=>]=%*[\"']%[^\"']%*[\"']", svg->att, svg->value)

#define read_tag_end \
	if (res < 0) {											\
		LOG("error parsing: %s\n", svg->att);				\
		return -1;											\
	}														\
	if (res == 1) {											\
		if (getwc(f) != L'>') {								\
			LOG("parsing error, expecting '>'\n");			\
			return -1;										\
		}													\
		return 0;											\
	}														\
	if (getwc(f) != L'>') {									\
		LOG("parsing error, expecting '>'\n");			\
		return -1;											\
	}														\
	res = read_tag(svg, f, hasStroke, hasFill, stroke, fill);\



uint32_t parseColorName (const wchar_t* name) {
	int valLenght = wcslen (name);

	switch(tolower(name[0])) {
	case L'a':
		switch(tolower(name[1])) {
		case L'l':
			return 0xFFFFF8F0;//AliceBlue
		case L'n':
			return 0xFFD7EBFA;//AntiqueWhite
		case L'q':
			if (!wcsncasecmp (&name[2],L"ua",2)) {//down
				if (valLenght == 4)
					return 0xFFFFFF00;//Aqua
				if (!wcscasecmp (&name[4],L"marine"))
					return 0xFFD4FF7F;//Aquamarine
				else
					return 0;//UNKNOWN COLOR
			}
			break;
		case L'z':
			return 0xFFFFFFF0;//Azure
		}
		break;
	case L'b':
		switch(tolower(name[1])) {
		case L'e':
			return 0xFFDCF5F5;//Beige
		case L'i':
			return 0xFFC4E4FF;//Bisque
		case L'l':
			switch(tolower(name[2])) {
			case L'a':
				switch(tolower(name[3])) {
				case L'c':
					return 0xFF000000;//Black
				case L'n':
					return 0xFFCDEBFF;//BlanchedAlmond
				}
				break;
			case L'u':
				if (tolower(name[3]) == L'e') {//down
					if (valLenght == 4)
						return 0xFFFF0000;//Blue
					if (!wcscasecmp (&name[4],L"violet"))
						return 0xFFE22B8A;//BlueViolet
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
			break;
		case L'r':
			return 0xFF2A2AA5;//Brown
		case L'u':
			return 0xFF87B8DE;//Burlywood
		}
		break;
	case L'c':
		switch(tolower(name[1])) {
		case L'a':
			return 0xFFA09E5F;//CadetBlue
		case L'h':
			switch(tolower(name[2])) {
			case L'a':
				return 0xFF00FF7F;//Chartreuse
			case L'o':
				return 0xFF1E69D2;//Chocolate
			}
			break;
		case L'o':
			if (tolower(name[2]) == L'r') {//up
				switch(tolower(name[3])) {
				case L'a':
					return 0xFF507FFF;//Coral
				case L'n':
					switch(tolower(name[4])) {
					case L'f':
						return 0xFFED9564;//Cornflower
					case L's':
						return 0xFFDCF8FF;//Cornsilk
					}
					break;
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case L'r':
			return 0xFF3C14DC;//Crimson
		case L'y':
			return 0xFFFFFF00;//Cyan
		}
		break;
	case L'd':
		switch(tolower(name[1])) {
		case L'a':
			if (!wcsncasecmp (&name[2],L"rk",2)) {//up
				switch(tolower(name[4])) {
				case L'b':
					return 0xFF8B0000;//DarkBlue
				case L'c':
					return 0xFF8B8B00;//DarkCyan
				case L'g':
					switch(tolower(name[5])) {
					case L'o':
						return 0xFF0B86B8;//DarkGoldenrod
					case L'r':
						switch(tolower(name[6])) {
						case L'a':
							return 0xFFA9A9A9;//DarkGray
						case L'e':
							return 0xFF006400;//DarkGreen
						}
						break;
					}
					break;
				case L'k':
					return 0xFF6BB7BD;//DarkKhaki
				case L'm':
					return 0xFF8B008B;//DarkMagenta
				case L'o':
					switch(tolower(name[5])) {
					case L'l':
						return 0xFF2F6B55;//DarkOliveGreen
					case L'r':
						switch(tolower(name[6])) {
						case L'a':
							return 0xFF008CFF;//DarkOrange
						case L'c':
							return 0xFFCC3299;//DarkOrchid
						}
						break;
					}
					break;
				case L'r':
					return 0xFF00008B;//DarkRed
				case L's':
					switch(tolower(name[5])) {
					case L'a':
						return 0xFF7A96E9;//DarkSalmon
					case L'e':
						return 0xFF8FBC8F;//DarkSeaGreen
					case L'l':
						if (!wcsncasecmp (&name[6],L"ate",3)) {//up
							switch(tolower(name[9])) {
							case L'b':
								return 0xFF8B3D48;//DarkSlateBlue
							case L'g':
								return 0xFF4F4F2F;//DarkSlateGray
							}
						} else
							return 0;//UNKNOWN COLOR
						break;
					}
					break;
				case L't':
					return 0xFFD1CE00;//DarkTurquoise
				case L'v':
					return 0xFFD30094;//DarkViolet
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case L'e':
			if (!wcsncasecmp (&name[2],L"ep",2)) {//up
				switch(tolower(name[4])) {
				case L'p':
					return 0xFF9314FF;//DeepPink
				case L's':
					return 0xFFFFBF00;//DeepSkyBlue
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case L'i':
			return 0xFF696969;//DimGray
		case L'o':
			return 0xFFFF901E;//DodgerBlue
		}
		break;
	case L'f':
		switch(tolower(name[1])) {
		case L'i':
			return 0xFF2222B2;//Firebrick
		case L'l':
			return 0xFFF0FAFF;//FloralWhite
		case L'o':
			return 0xFF228B22;//ForestGreen
		case L'u':
			return 0xFFFF00FF;//Fuchsia
		}
		break;
	case L'g':
		switch(tolower(name[1])) {
		case L'a':
			return 0xFFDCDCDC;//Gainsboro
		case L'h':
			return 0xFFFFF8F8;//GhostWhite
		case L'o':
			if (!wcsncasecmp (&name[2],L"ld",2)) {//down
				if (valLenght == 4)
					return 0xFF00D7FF;//Gold
				if (!wcscasecmp (&name[4],L"enrod"))
					return 0xFF20A5DA;//Goldenrod
				else
					return 0;//UNKNOWN COLOR
			}
			break;
		case L'r':
			switch(tolower(name[2])) {
			case L'a':
				return 0xFFBEBEBE;//Gray
			case L'e':
				if (!wcsncasecmp (&name[3],L"en",2)) {//down
					if (valLenght == 5)
						return 0xFF00FF00;//Green
					if (!wcscasecmp (&name[5],L"yellow"))
						return 0xFF2FFFAD;//GreenYellow
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
			break;
		}
		break;
	case L'h':
		if (tolower(name[1]) == L'o') {//up
			switch(tolower(name[2])) {
			case L'n':
				return 0xFFF0FFF0;//Honeydew
			case L't':
				return 0xFFB469FF;//HotPink
			}
		} else
			return 0;//UNKNOWN COLOR
		break;
	case L'i':
		switch(tolower(name[1])) {
		case L'n':
			if (!wcsncasecmp (&name[2],L"di",2)) {//up
				switch(tolower(name[4])) {
				case L'a':
					return 0xFF5C5CCD;//IndianRed
				case L'g':
					return 0xFF82004B;//Indigo
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case L'v':
			return 0xFFF0FFFF;//Ivory
		}
		break;
	case L'k':
		return 0xFF8CE6F0;//Khaki
	case L'l':
		switch(tolower(name[1])) {
		case L'a':
			switch(tolower(name[2])) {
			case L'v':
				if (!wcsncasecmp (&name[3],L"ender",5)) {//down
					if (valLenght == 8)
						return 0xFFFAE6E6;//Lavender
					if (!wcscasecmp (&name[8],L"blush"))
						return 0xFFF5F0FF;//LavenderBlush
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			case L'w':
				return 0xFF00FC7C;//LawnGreen
			}
			break;
		case L'e':
			return 0xFFCDFAFF;//LemonChiffon
		case L'i':
			switch(tolower(name[2])) {
			case L'g':
				if (!wcsncasecmp (&name[3],L"ht",2)) {//up
					switch(tolower(name[5])) {
					case L'b':
						return 0xFFE6D8AD;//LightBlue
					case L'c':
						switch(tolower(name[6])) {
						case L'o':
							return 0xFF8080F0;//LightCoral
						case L'y':
							return 0xFFFFFFE0;//LightCyan
						}
						break;
					case L'g':
						switch(tolower(name[6])) {
						case L'o':
							return 0xFFD2FAFA;//LightGoldenrod
						case L'r':
							switch(tolower(name[7])) {
							case L'a':
								return 0xFFD3D3D3;//LightGray
							case L'e':
								return 0xFF90EE90;//LightGreen
							}
							break;
						}
						break;
					case L'p':
						return 0xFFC1B6FF;//LightPink
					case L's':
						switch(tolower(name[6])) {
						case L'a':
							return 0xFF7AA0FF;//LightSalmon
						case L'e':
							return 0xFFAAB220;//LightSeaGreen
						case L'k':
							return 0xFFFACE87;//LightSkyBlue
						case L'l':
							return 0xFF998877;//LightSlateGray
						case L't':
							return 0xFFDEC4B0;//LightSteelBlue
						}
						break;
					case L'y':
						return 0xFFE0FFFF;//LightYellow
					}
				} else
					return 0;//UNKNOWN COLOR
				break;
			case L'm':
				if (tolower(name[3]) == L'e') {//down
					if (valLenght == 4)
						return 0xFF00FF00;//Lime
					if (!wcscasecmp (&name[4],L"green"))
						return 0xFF32CD32;//LimeGreen
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			case L'n':
				return 0xFFE6F0FA;//Linen
			}
			break;
		}
		break;
	case L'm':
		switch(tolower(name[1])) {
		case L'a':
			switch(tolower(name[2])) {
			case L'g':
				return 0xFFFF00FF;//Magenta
			case L'r':
				return 0xFF6030B0;//Maroon
			}
			break;
		case L'e':
			if (!wcsncasecmp (&name[2],L"dium",4)) {//up
				switch(tolower(name[6])) {
				case L'a':
					return 0xFFAACD66;//MediumAquamarine
				case L'b':
					return 0xFFCD0000;//MediumBlue
				case L'o':
					return 0xFFD355BA;//MediumOrchid
				case L'p':
					return 0xFFDB7093;//MediumPurple
				case L's':
					switch(tolower(name[7])) {
					case L'e':
						return 0xFF71B33C;//MediumSeaGreen
					case L'l':
						return 0xFFEE687B;//MediumSlateBlue
					case L'p':
						return 0xFF9AFA00;//MediumSpringGreen
					}
					break;
				case L't':
					return 0xFFCCD148;//MediumTurquoise
				case L'v':
					return 0xFF8515C7;//MediumVioletRed
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case L'i':
			switch(tolower(name[2])) {
			case L'd':
				return 0xFF701919;//MidnightBlue
			case L'n':
				return 0xFFFAFFF5;//MintCream
			case L's':
				return 0xFFE1E4FF;//MistyRose
			}
			break;
		case L'o':
			return 0xFFB5E4FF;//Moccasin
		}
		break;
	case L'n':
		if (!wcsncasecmp (&name[1],L"av",2)) {//up
			switch(tolower(name[3])) {
			case L'a':
				return 0xFFADDEFF;//NavajoWhite
			case L'y':
				return 0xFF800000;//Navy
			}
		} else
			return 0;//UNKNOWN COLOR
		break;
	case L'o':
		switch(tolower(name[1])) {
		case L'l':
			switch(tolower(name[2])) {
			case L'd':
				return 0xFFE6F5FD;//OldLace
			case L'i':
				if (!wcsncasecmp (&name[3],L"ve",2)) {//down
					if (valLenght == 5)
						return 0xFF008080;//Olive
					if (!wcscasecmp (&name[5],L"drab"))
						return 0xFF238E6B;//OliveDrab
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
			break;
		case L'r':
			switch(tolower(name[2])) {
			case L'a':
				if (!wcsncasecmp (&name[3],L"nge",3)) {//down
					if (valLenght == 6)
						return 0xFF00A5FF;//Orange
					if (!wcscasecmp (&name[6],L"red"))
						return 0xFF0045FF;//OrangeRed
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			case L'c':
				return 0xFFD670DA;//Orchid
			}
			break;
		}
		break;
	case L'p':
		switch(tolower(name[1])) {
		case L'a':
			switch(tolower(name[2])) {
			case L'l':
				if (tolower(name[3]) == L'e') {//up
					switch(tolower(name[4])) {
					case L'g':
						switch(tolower(name[5])) {
						case L'o':
							return 0xFFAAE8EE;//PaleGoldenrod
						case L'r':
							return 0xFF98FB98;//PaleGreen
						}
						break;
					case L't':
						return 0xFFEEEEAF;//PaleTurquoise
					case L'v':
						return 0xFF9370DB;//PaleVioletRed
					}
				} else
					return 0;//UNKNOWN COLOR
				break;
			case L'p':
				return 0xFFD5EFFF;//PapayaWhip
			}
			break;
		case L'e':
			switch(tolower(name[2])) {
			case L'a':
				return 0xFFB9DAFF;//PeachPuff
			case L'r':
				return 0xFF3F85CD;//Peru
			}
			break;
		case L'i':
			return 0xFFCBC0FF;//Pink
		case L'l':
			return 0xFFDDA0DD;//Plum
		case L'o':
			return 0xFFE6E0B0;//PowderBlue
		case L'u':
			return 0xFFF020A0;//Purple
		}
		break;
	case L'r':
		switch(tolower(name[1])) {
		case L'e':
			return 0xFF0000FF;//Red
		case L'o':
			switch(tolower(name[2])) {
			case L's':
				return 0xFF8F8FBC;//RosyBrown
			case L'y':
				return 0xFFE16941;//RoyalBlue
			}
			break;
		}
		break;
	case L's':
		switch(tolower(name[1])) {
		case L'a':
			switch(tolower(name[2])) {
			case L'd':
				return 0xFF13458B;//SaddleBrown
			case L'l':
				return 0xFF7280FA;//Salmon
			case L'n':
				return 0xFF60A4F4;//SandyBrown
			}
			break;
		case L'e':
			if (tolower(name[2]) == L'a') {//up
				switch(tolower(name[3])) {
				case L'g':
					return 0xFF578B2E;//SeaGreen
				case L's':
					return 0xFFEEF5FF;//Seashell
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case L'i':
			switch(tolower(name[2])) {
			case L'e':
				return 0xFF2D52A0;//Sienna
			case L'l':
				return 0xFFC0C0C0;//Silver
			}
			break;
		case L'k':
			return 0xFFEBCE87;//SkyBlue
		case L'l':
			if (!wcsncasecmp (&name[2],L"ate",3)) {//up
				switch(tolower(name[5])) {
				case L'b':
					return 0xFFCD5A6A;//SlateBlue
				case L'g':
					return 0xFF908070;//SlateGray
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case L'n':
			return 0xFFFAFAFF;//Snow
		case L'p':
			return 0xFF7FFF00;//SpringGreen
		case L't':
			return 0xFFB48246;//SteelBlue
		}
		break;
	case L't':
		switch(tolower(name[1])) {
		case L'a':
			return 0xFF8CB4D2;//Tan
		case L'e':
			return 0xFF808000;//Teal
		case L'h':
			return 0xFFD8BFD8;//Thistle
		case L'o':
			return 0xFF4763FF;//Tomato
		case L'u':
			return 0xFFD0E040;//Turquoise
		}
		break;
	case L'v':
		return 0xFFEE82EE;//Violet
	case L'w':
		if (tolower(name[1]) == L'h') {//up
			switch(tolower(name[2])) {
			case L'e':
				return 0xFFB3DEF5;//Wheat
			case L'i':
				if (!wcsncasecmp (&name[3],L"te",2)) {//down
					if (valLenght == 5)
						return 0xFFFFFFFF;//White
					if (!wcscasecmp (&name[5],L"smoke"))
						return 0xFFF5F5F5;//WhiteSmoke
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
		} else
			return 0;//UNKNOWN COLOR
		break;
	case L'y':
		if (!wcsncasecmp (&name[1],L"ellow",5)) {//down
			if (valLenght == 6)
				return 0xFF00FFFF;//Yellow
			if (!wcscasecmp (&name[6],L"green"))
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
		fwscanf(f, L",");
		if (fwscanf(f, L" %f ", pF)!=1) {
			va_end(args);
			return false;
		}
	}
	va_end(args);
	return true;
}
float parse_lenghtOrPercentage (const wchar_t* measure) {
	float m = 0;
	res = sscanf(measure, "%f", &m);
	return m;
}
bool parse_color (const wchar_t* colorString, bool* isEnabled, uint32_t* colorValue) {

	if (colorString[0] == '#') {
		char color[7];
		res = swscanf(&colorString[1], L" %[0-9A-Fa-f]", color);
		char hexColorString[12];
		if (strlen (color) == 3)
			sprintf(hexColorString, "0xff%c%c%c%c%c%c", color[2],color[2],color[1],color[1],color[0],color[0]);
		else
			sprintf(hexColorString, "0xff%c%c%c%c%c%c", color[4],color[5],color[2],color[3],color[0],color[1]);
		sscanf(hexColorString, "%x", colorValue);
		*isEnabled = true;
		return true;
	}
	char r = 0, g = 0, b = 0, a = 0;
	if (swscanf(colorString, L"rgb ( %d%*[ ,]%d%*[ ,]%d )", &r, &g, &b))
		*colorValue = 0xff000000 + (b << 16) + (g << 8) + r;
	else if (swscanf(colorString, L"rgba ( %d%*[ ,]%d%*[ ,]%d%*[ ,]%d )", &r, &g, &b, &a))
		*colorValue = (a << 24) + (b << 16) + (g << 8) + r;
	else if (!wcscasecmp (colorString, L"none"))
		*colorValue = 0;
	else if (wcscasecmp (colorString, L"currentColor"))
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
	if (!wcscasecmp(svg->att, L"fill"))
		parse_color (svg->value, hasFill, fill);
	else if (!wcscasecmp(svg->att, L"fill-rule")) {
		if (!wcscasecmp(svg->value, L"evenodd"))
			vkvg_set_fill_rule(svg->ctx, VKVG_FILL_RULE_EVEN_ODD);
		else
			vkvg_set_fill_rule(svg->ctx, VKVG_FILL_RULE_NON_ZERO);
	}else if (!wcscasecmp(svg->att, L"stroke"))
		parse_color (svg->value, hasStroke, stroke);
	else if (!wcscasecmp (svg->att, L"stroke-width"))
		vkvg_set_line_width(svg->ctx, parse_lenghtOrPercentage (svg->value));
	else if (!wcscasecmp (svg->att, L"stroke-linecap")) {
		if (!wcscasecmp(svg->value, L"round"))
			vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_ROUND);
		else if (!wcscasecmp(svg->value, L"square"))
			vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_SQUARE);
		else if (!wcscasecmp(svg->value, L"butt"))
			vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_BUTT);
	} else if (!wcscasecmp (svg->att, L"stroke-linejoin")) {
		if (!wcscasecmp(svg->value, L"round"))
			vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_ROUND);
		else if (!wcscasecmp(svg->value, L"bevel"))
			vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_BEVEL);
		else if (!wcscasecmp(svg->value, L"miter"))
			vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_MITER);
	} else if (!wcscasecmp (svg->att, L"transform")) {
		FILE *tmp = fmemopen(svg->value, wcslen (svg->value), "r");
		wchar_t transform[16];
		while (!feof(tmp)) {
			if (fwscanf(tmp, L" %[^(](", transform) == 1) {
				if (!wcscasecmp (transform, L"matrix")) {
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
				wchar_t c = getc(tmp);
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
			wint_t c;
			if (!wcscasecmp (svg->att, L"d")) {
				FILE *tmp = fmemopen(svg->value, wcslen (svg->value), "r");
				bool parseError = false, result = false;
				while (!(parseError || feof(tmp))) {
					if (!repeat && fwscanf(tmp, L" %lc ", &c) != 1) {
						LOG ("error parsing path: expectin char\n");
						break;
					}

					switch (c) {
					case L'M':
						if (parse_floats(tmp, 2, &x, &y))
							vkvg_move_to (svg->ctx, x, y);
						else
							parseError = true;
						break;
					case L'm':
						if (parse_floats(tmp, 2, &x, &y))
							vkvg_rel_move_to (svg->ctx, x, y);
						else
							parseError = true;
						break;
					case L'L':
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
					case L'l':
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
					case L'H':
						if (parse_floats(tmp, 1, &x)) {
							vkvg_get_current_point (svg->ctx, &c1x, &c1y);
							vkvg_line_to (svg->ctx, x, c1y);
						} else
							parseError = true;
						break;
					case L'h':
						if (parse_floats(tmp, 1, &x))
							vkvg_rel_line_to (svg->ctx, x, 0);
						else
							parseError = true;
						break;
					case L'V':
						if (parse_floats(tmp, 1, &y)) {
							vkvg_get_current_point (svg->ctx, &c1x, &c1y);
							vkvg_line_to (svg->ctx, c1x, y);
						} else
							parseError = true;
						break;
					case L'v':
						if (parse_floats(tmp, 1, &y))
							vkvg_rel_line_to (svg->ctx, 0, y);
						else
							parseError = true;
						break;
					case L'Q':
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
					case L'q':
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
					case L'T':
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
					case L't':
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
					case L'C':
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
					case L'c':
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
					case L'S':
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
					case L's':
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
					case L'A'://rx ry x-axis-rotation large-arc-flag sweep-flag x y
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
					case L'a'://rx ry x-axis-rotation large-arc-flag sweep-flag x y
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
					case L'z':
					case L'Z':
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
		if (!wcscasecmp (svg->att, L"viewBox")) {
			res = swscanf(svg->value, L" %d%*1[ ,]%d%*1[ ,]%d%*1[ ,]%d", &x, &y, &w, &h);
			if (res!=4)
				return -1;
			hasViewBox = true;
		} else if (!wcscasecmp (svg->att, L"width"))
			width = parse_lenghtOrPercentage (svg->value);
		else if (!wcscasecmp (svg->att, L"height"))
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
																	wcscasecmp (svg->att, L"viewBox") &&
																	wcscasecmp (svg->att, L"width") &&
																	wcscasecmp (svg->att, L"height"))
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
		res = fwscanf(f, L" <%[^> ]", svg->elt);
		if (res < 0)
			return 0;
		if (!res) {
			res = fwscanf(f, L"%*[^<]<%[^> ]", svg->elt);
			if (!res) {
				LOG("element name parsing error (%s)\n", svg->elt);
				return -1;
			}
		}
		if (svg->elt[0] == L'/') {
			if (getwc(f) != L'>') {
				LOG("parsing error, expecting '>'\n");
				return -1;
			}
			//read_attributes_ptr = NULL;
			return 0;
		}
		if (svg->elt[0] == '!') {
			if (!wcscasecmp (svg->elt, L"!DOCTYPE")) {
				while (!feof (f))
					if (fgetc(f)=='>')
						break;;
			} if (!wcsncasecmp (svg->elt, L"!--", 3)) {
				while (!feof (f)) {
					if (fgetwc(f)==L'-') {
						if (fgetwc(f)==L'-' && fgetwc(f)==L'>')
							break;
						else
							fseek (f, -1, SEEK_CUR);
					}
				}
			}
			continue;
		}
		if (!wcscasecmp (svg->elt, L"svg"))
			res = read_svg_attributes	(svg, f, hasStroke, hasFill, stroke, fill);
		else if (!wcscasecmp (svg->elt, L"g")) {
			vkvg_save (svg->ctx);
			res = read_g_attributes		(svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!wcscasecmp (svg->elt, L"rect")) {
			vkvg_save (svg->ctx);
			res = read_rect_attributes	(svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!wcscasecmp (svg->elt, L"line")) {
			vkvg_save (svg->ctx);
			res = read_line_attributes	(svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!wcscasecmp (svg->elt, L"circle")) {
			vkvg_save (svg->ctx);
			res = read_circle_attributes (svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!wcscasecmp (svg->elt, L"polyline")) {
			vkvg_save (svg->ctx);
			res = read_polyline_attributes (svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else if (!wcscasecmp (svg->elt, L"path")) {
			vkvg_save (svg->ctx);
			res = read_path_attributes	(svg, f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (svg->ctx);
		} else
			res = read_attributes		(svg, f, hasStroke, hasFill, stroke, fill);
	}
	return res;
}

VkvgSurface parse_svg_file (VkvgDevice dev, const char* filename, uint32_t width, uint32_t height) {
	if (!setlocale(LC_CTYPE, "")) {
		printf( "Can't set the specified locale! "
				"Check LANG, LC_CTYPE, LC_ALL.\n");
		return NULL;
	}

	FILE* f = fopen(filename, "r");
	if (f == NULL){
		perror ("vkvg_svg: file not found");
		return NULL;
	}

	svg_context svg = {0};
	svg.dev		= dev;
	svg.width	= width;
	svg.height	= height;

//	res = fwscanf(f, L"%ls", svg.elt);

	/*wint_t c = fgetwc(f);
	while (c != WEOF) {
		c = fgetwc(f);
	}*/

	read_tag (&svg, f, false, true, 0xffffffff, 0xffffffff);

	fclose(f);
	return svg.surf;
}
