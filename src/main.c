#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <vkvg.h>
#include "vkengine.h"
#include <stdarg.h>
#include <ctype.h>


static int res;
VkvgDevice dev;
static VkvgSurface newSvgSurf = NULL, svgSurf = NULL;
static VkvgContext ctx;
double scale = 2;

int read_tag (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill);

#define get_attribute fscanf(f, " %[^=>]=%*[\"']%[^\"']%*[\"']", att, value)

#define read_tag_end \
	if (res < 0) {								\
		printf("error parsing: %s\n", att);		\
		return -1;								\
	}											\
	if (res == 1) {								\
		if (getc(f) != '>') {					\
			printf("parsing error, expecting '>'\n");\
			return -1;							\
		}										\
		return 0;								\
	}											\
	if (getc(f) != '>') {						\
		printf("parsing error, expecting '>'\n");\
		return -1;								\
	}											\
	res = read_tag(f, hasStroke, hasFill, stroke, fill);\

//static int (*read_attributes_ptr)(FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill);

/*static uint32_t strokes[32];
static uint32_t fills[32];
static int strokesPtr = -1 fillsPtr = -1;

void push_stroke_color (uint32_t stroke) {
	strokes[++strokesPtr] = stroke;
}*/
static char elt[128];
static char att[1024];
static char value[1024*1024];


uint32_t parseColorName () {
	int valLenght = strlen (value);


	switch(tolower(value[0])) {
	case 'a':
		switch(tolower(value[1])) {
		case 'l':
			return 0xFFFFF8F0;//AliceBlue
		case 'n':
			return 0xFFD7EBFA;//AntiqueWhite
		case 'q':
			if (!strncasecmp(&value[2],"ua",2)) {//down
				if (valLenght == 4)
					return 0xFFFFFF00;//Aqua
				if (!strcasecmp(&value[4],"marine"))
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
		switch(tolower(value[1])) {
		case 'e':
			return 0xFFDCF5F5;//Beige
		case 'i':
			return 0xFFC4E4FF;//Bisque
		case 'l':
			switch(tolower(value[2])) {
			case 'a':
				switch(tolower(value[3])) {
				case 'c':
					return 0xFF000000;//Black
				case 'n':
					return 0xFFCDEBFF;//BlanchedAlmond
				}
				break;
			case 'u':
				if (tolower(value[3]) == 'e') {//down
					if (valLenght == 4)
						return 0xFFFF0000;//Blue
					if (!strcasecmp(&value[4],"violet"))
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
		switch(tolower(value[1])) {
		case 'a':
			return 0xFFA09E5F;//CadetBlue
		case 'h':
			switch(tolower(value[2])) {
			case 'a':
				return 0xFF00FF7F;//Chartreuse
			case 'o':
				return 0xFF1E69D2;//Chocolate
			}
			break;
		case 'o':
			if (tolower(value[2]) == 'r') {//up
				switch(tolower(value[3])) {
				case 'a':
					return 0xFF507FFF;//Coral
				case 'n':
					switch(tolower(value[4])) {
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
		switch(tolower(value[1])) {
		case 'a':
			if (!strncasecmp(&value[2],"rk",2)) {//up
				switch(tolower(value[4])) {
				case 'b':
					return 0xFF8B0000;//DarkBlue
				case 'c':
					return 0xFF8B8B00;//DarkCyan
				case 'g':
					switch(tolower(value[5])) {
					case 'o':
						return 0xFF0B86B8;//DarkGoldenrod
					case 'r':
						switch(tolower(value[6])) {
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
					switch(tolower(value[5])) {
					case 'l':
						return 0xFF2F6B55;//DarkOliveGreen
					case 'r':
						switch(tolower(value[6])) {
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
					switch(tolower(value[5])) {
					case 'a':
						return 0xFF7A96E9;//DarkSalmon
					case 'e':
						return 0xFF8FBC8F;//DarkSeaGreen
					case 'l':
						if (!strncasecmp(&value[6],"ate",3)) {//up
							switch(tolower(value[9])) {
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
			if (!strncasecmp(&value[2],"ep",2)) {//up
				switch(tolower(value[4])) {
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
		switch(tolower(value[1])) {
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
		switch(tolower(value[1])) {
		case 'a':
			return 0xFFDCDCDC;//Gainsboro
		case 'h':
			return 0xFFFFF8F8;//GhostWhite
		case 'o':
			if (!strncasecmp(&value[2],"ld",2)) {//down
				if (valLenght == 4)
					return 0xFF00D7FF;//Gold
				if (!strcasecmp(&value[4],"enrod"))
					return 0xFF20A5DA;//Goldenrod
				else
					return 0;//UNKNOWN COLOR
			}
			break;
		case 'r':
			switch(tolower(value[2])) {
			case 'a':
				return 0xFFBEBEBE;//Gray
			case 'e':
				if (!strncasecmp(&value[3],"en",2)) {//down
					if (valLenght == 5)
						return 0xFF00FF00;//Green
					if (!strcasecmp(&value[5],"yellow"))
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
		if (tolower(value[1]) == 'o') {//up
			switch(tolower(value[2])) {
			case 'n':
				return 0xFFF0FFF0;//Honeydew
			case 't':
				return 0xFFB469FF;//HotPink
			}
		} else
			return 0;//UNKNOWN COLOR
		break;
	case 'i':
		switch(tolower(value[1])) {
		case 'n':
			if (!strncasecmp(&value[2],"di",2)) {//up
				switch(tolower(value[4])) {
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
		switch(tolower(value[1])) {
		case 'a':
			switch(tolower(value[2])) {
			case 'v':
				if (!strncasecmp(&value[3],"ender",5)) {//down
					if (valLenght == 8)
						return 0xFFFAE6E6;//Lavender
					if (!strcasecmp(&value[8],"blush"))
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
			switch(tolower(value[2])) {
			case 'g':
				if (!strncasecmp(&value[3],"ht",2)) {//up
					switch(tolower(value[5])) {
					case 'b':
						return 0xFFE6D8AD;//LightBlue
					case 'c':
						switch(tolower(value[6])) {
						case 'o':
							return 0xFF8080F0;//LightCoral
						case 'y':
							return 0xFFFFFFE0;//LightCyan
						}
						break;
					case 'g':
						switch(tolower(value[6])) {
						case 'o':
							return 0xFFD2FAFA;//LightGoldenrod
						case 'r':
							switch(tolower(value[7])) {
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
						switch(tolower(value[6])) {
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
				if (tolower(value[3]) == 'e') {//down
					if (valLenght == 4)
						return 0xFF00FF00;//Lime
					if (!strcasecmp(&value[4],"green"))
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
		switch(tolower(value[1])) {
		case 'a':
			switch(tolower(value[2])) {
			case 'g':
				return 0xFFFF00FF;//Magenta
			case 'r':
				return 0xFF6030B0;//Maroon
			}
			break;
		case 'e':
			if (!strncasecmp(&value[2],"dium",4)) {//up
				switch(tolower(value[6])) {
				case 'a':
					return 0xFFAACD66;//MediumAquamarine
				case 'b':
					return 0xFFCD0000;//MediumBlue
				case 'o':
					return 0xFFD355BA;//MediumOrchid
				case 'p':
					return 0xFFDB7093;//MediumPurple
				case 's':
					switch(tolower(value[7])) {
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
			switch(tolower(value[2])) {
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
		if (!strncasecmp(&value[1],"av",2)) {//up
			switch(tolower(value[3])) {
			case 'a':
				return 0xFFADDEFF;//NavajoWhite
			case 'y':
				return 0xFF800000;//Navy
			}
		} else
			return 0;//UNKNOWN COLOR
		break;
	case 'o':
		switch(tolower(value[1])) {
		case 'l':
			switch(tolower(value[2])) {
			case 'd':
				return 0xFFE6F5FD;//OldLace
			case 'i':
				if (!strncasecmp(&value[3],"ve",2)) {//down
					if (valLenght == 5)
						return 0xFF008080;//Olive
					if (!strcasecmp(&value[5],"drab"))
						return 0xFF238E6B;//OliveDrab
					else
						return 0;//UNKNOWN COLOR
				}
				break;
			}
			break;
		case 'r':
			switch(tolower(value[2])) {
			case 'a':
				if (!strncasecmp(&value[3],"nge",3)) {//down
					if (valLenght == 6)
						return 0xFF00A5FF;//Orange
					if (!strcasecmp(&value[6],"red"))
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
		switch(tolower(value[1])) {
		case 'a':
			switch(tolower(value[2])) {
			case 'l':
				if (tolower(value[3]) == 'e') {//up
					switch(tolower(value[4])) {
					case 'g':
						switch(tolower(value[5])) {
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
			switch(tolower(value[2])) {
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
		switch(tolower(value[1])) {
		case 'e':
			return 0xFF0000FF;//Red
		case 'o':
			switch(tolower(value[2])) {
			case 's':
				return 0xFF8F8FBC;//RosyBrown
			case 'y':
				return 0xFFE16941;//RoyalBlue
			}
			break;
		}
		break;
	case 's':
		switch(tolower(value[1])) {
		case 'a':
			switch(tolower(value[2])) {
			case 'd':
				return 0xFF13458B;//SaddleBrown
			case 'l':
				return 0xFF7280FA;//Salmon
			case 'n':
				return 0xFF60A4F4;//SandyBrown
			}
			break;
		case 'e':
			if (tolower(value[2]) == 'a') {//up
				switch(tolower(value[3])) {
				case 'g':
					return 0xFF578B2E;//SeaGreen
				case 's':
					return 0xFFEEF5FF;//Seashell
				}
			} else
				return 0;//UNKNOWN COLOR
			break;
		case 'i':
			switch(tolower(value[2])) {
			case 'e':
				return 0xFF2D52A0;//Sienna
			case 'l':
				return 0xFFC0C0C0;//Silver
			}
			break;
		case 'k':
			return 0xFFEBCE87;//SkyBlue
		case 'l':
			if (!strncasecmp(&value[2],"ate",3)) {//up
				switch(tolower(value[5])) {
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
		switch(tolower(value[1])) {
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
		if (tolower(value[1]) == 'h') {//up
			switch(tolower(value[2])) {
			case 'e':
				return 0xFFB3DEF5;//Wheat
			case 'i':
				if (!strncasecmp(&value[3],"te",2)) {//down
					if (valLenght == 5)
						return 0xFFFFFFFF;//White
					if (!strcasecmp(&value[5],"smoke"))
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
		if (!strncasecmp(&value[1],"ellow",5)) {//down
			if (valLenght == 6)
				return 0xFF00FFFF;//Yellow
			if (!strcasecmp(&value[6],"green"))
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
uint32_t parse_color (const char* colorString, bool* isEnabled) {
	uint32_t colorValue = 0;
	if (colorString[0] == '#') {
		char color[6];
		res = sscanf(&colorString[1], " %[0-9A-Fa-f]", color);
		char hexColorString[11];
		if (strlen(color) == 3)
			sprintf(hexColorString, "0xff%c%c%c%c%c%c", color[2],color[2],color[1],color[1],color[0],color[0]);
		else
			sprintf(hexColorString, "0xff%c%c%c%c%c%c", color[4],color[5],color[2],color[3],color[0],color[1]);
		sscanf(hexColorString, "%x", &colorValue);
		*isEnabled = true;
		return colorValue;
	}
	uint32_t r = 0, g = 0, b = 0, a = 0;
	if (sscanf(colorString, "rgb ( %d%*[ ,]%d%*[ ,]%d )", &r, &g, &b))
		colorValue = 0xff000000 + (b << 16) + (g << 8) + r;
	else if (sscanf(colorString, "rgba ( %d%*[ ,]%d%*[ ,]%d%*[ ,]%d )", &r, &g, &b, &a))
		colorValue = (a << 24) + (b << 16) + (g << 8) + r;
	else if (!strcmp (colorString, "none"))
		colorValue = 0;
	else
		colorValue = parseColorName();
	*isEnabled = colorValue > 0;
	return colorValue;
}
int draw (bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	if (hasFill) {
		vkvg_set_source_color(ctx, fill);
		if (hasStroke) {
			vkvg_fill_preserve(ctx);
			vkvg_set_source_color(ctx, stroke);
			vkvg_stroke(ctx);
		} else
			vkvg_fill (ctx);
	} else if (hasStroke) {
		vkvg_set_source_color(ctx, stroke);
		vkvg_stroke(ctx);
	}
}

int read_common_attributes (bool *hasStroke, bool *hasFill, uint32_t *stroke, uint32_t *fill) {
	if (!strcmp(att, "id")) {
		if (!strcmp(value, "g746"))
			printf("");

	}else if (!strcmp(att, "fill"))
		*fill = parse_color (value, hasFill);
	else if (!strcmp(att, "fill-rule")) {
		if (!strcmp(value, "evenodd"))
			vkvg_set_fill_rule(ctx, VKVG_FILL_RULE_EVEN_ODD);
		else
			vkvg_set_fill_rule(ctx, VKVG_FILL_RULE_NON_ZERO);
	}else if (!strcmp(att, "stroke"))
		*stroke = parse_color (value, hasStroke);
	else if (!strcmp (att, "stroke-width"))
		vkvg_set_line_width(ctx, parse_lenghtOrPercentage (value));
	else if (!strcmp (att, "transform")) {
		FILE *tmp = fmemopen(value, strlen (value), "r");
		char transform[16];
		while (!feof(tmp)) {
			if (fscanf(tmp, " %[^(](", transform) == 1) {
				if (!strcmp (transform, "matrix")) {
					vkvg_matrix_t m, current, newMat;
					if (!parse_floats(tmp, 6, &m.xx, &m.yx, &m.xy, &m.yy, &m.x0, &m.y0)) {
						printf ("error parsing transformation matrix\n");
						break;
					}
					//vkvg_matrix_init_identity(&m);
					vkvg_get_matrix(ctx, &current);
					vkvg_matrix_multiply(&newMat, &m, &current);
					vkvg_set_matrix(ctx, &newMat);
				}
				char c = getc(tmp);
				if (c!=')') {
					printf ("error parsing transformation, expecting ')', having %c\n", c);
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
int read_svg_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	bool hasViewBox = false;
	int x, y, w, h, width = 0, height = 0;
	int startingPos = ftell(f);
	res = get_attribute;
	while (res == 2) {
		if (!strcmp(att, "viewBox")) {
			res = sscanf(value, " %d%*1[ ,]%d%*1[ ,]%d%*1[ ,]%d", &x, &y, &w, &h);
			if (res!=4)
				return -1;
			hasViewBox = true;
		} else if (!strcmp(att, "width"))
			width = parse_lenghtOrPercentage (value);
		else if (!strcmp(att, "height"))
			height = parse_lenghtOrPercentage (value);
		res = get_attribute;
	}
	int surfW, surfH;
	float xScale = 1, yScale = 1;
	if (width) {
		surfW = width;
		if (hasViewBox)
			xScale = (float)width / w;
	} else if (hasViewBox)
		surfW = w;
	else
		surfW = 512;
	if (height) {
		surfH = height;
		if (hasViewBox)
			yScale = (float)height / h;
	} else if (hasViewBox)
		surfH = h;
	else
		surfH = 512;

	newSvgSurf = vkvg_surface_create(dev, surfW * scale, surfH * scale);
	ctx = vkvg_create(newSvgSurf);
	vkvg_set_fill_rule(ctx, VKVG_FILL_RULE_EVEN_ODD);
	vkvg_scale(ctx, xScale * scale, yScale * scale);
	vkvg_clear(ctx);

	fseek (f, startingPos, SEEK_SET);
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill) &&
																	strcmp(att, "viewBox") &&
																	strcmp(att, "width") &&
																	strcmp(att, "height") &&
																	strcmp(att, "width"))
			printf("Unprocessed attribute: %s->%s\n", elt, att);
		res = get_attribute;
	}

	read_tag_end
	vkvg_destroy(ctx);
	return res;
}
int read_rect_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	int x = 0, y = 0, w = 0, h = 0, rx = 0, ry = 0;
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			if (!strcmp (att, "x"))
				x = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "y"))
				y = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "width"))
				w = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "height"))
				h = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "rx"))
				rx = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "ry"))
				ry = parse_lenghtOrPercentage(value);
			else
				printf("Unprocessed attribute: %s->%s\n", elt, att);
		}
		res = get_attribute;
	}
	if (w && h && (hasFill || hasStroke)) {
		vkvg_rectangle(ctx, x, y, w, h);
		draw (hasStroke, hasFill, stroke, fill);
	}
	read_tag_end
	return res;
}
int read_line_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	int x1 = 0, y1 = 0, x2 = 0, y2;
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			if (!strcmp (att, "x1"))
				x1 = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "y1"))
				y1 = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "x2"))
				x2 = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "y2"))
				y2 = parse_lenghtOrPercentage(value);
			else
				printf("Unprocessed attribute: %s->%s\n", elt, att);
		}
		res = get_attribute;
	}
	if (hasStroke) {
		vkvg_move_to(ctx, x1, y1);
		vkvg_move_to(ctx, x2, y2);
		draw (hasStroke, false, stroke, 0);
	}
	read_tag_end
	return res;
}
int read_circle_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	int cx = 0, cy = 0, r;
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			if (!strcmp (att, "cx"))
				cx = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "cy"))
				cy = parse_lenghtOrPercentage(value);
			else if (!strcmp (att, "r"))
				r = parse_lenghtOrPercentage(value);
			else
				printf("Unprocessed attribute: %s->%s\n", elt, att);
		}
		res = get_attribute;
	}
	if (r > 0 && (hasFill || hasStroke)) {
		vkvg_arc(ctx, cx, cy, r, 0, M_PI * 2);
		draw (hasStroke, hasFill, stroke, fill);
	}
	read_tag_end
	return res;
}
enum prevCmd {none, quad, cubic};
int read_path_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			float x, y, c1x, c1y, c2x, c2y, cpX, cpY;
			enum prevCmd prev = none;
			int repeat=0;
			char c;
			if (!strcmp (att, "d")) {
				FILE *tmp = fmemopen(value, strlen (value), "r");
				bool parseError = false, result = false;
				while (!(parseError || feof(tmp))) {
					if (!repeat && fscanf(tmp, " %c ", &c) != 1) {
						printf ("error parsing path: expectin char\n");
						break;
					}

					switch (c) {
					case 'M':
						if (parse_floats(tmp, 2, &x, &y))
							vkvg_move_to (ctx, x, y);
						else
							parseError = true;
						break;
					case 'm':
						if (parse_floats(tmp, 2, &x, &y))
							vkvg_rel_move_to (ctx, x, y);
						else
							parseError = true;
						break;
					case 'L':
						if (repeat)
							result = parse_floats(tmp, 1, &y);
						else
							result = parse_floats(tmp, 2, &x, &y);
						if (result) {
							vkvg_line_to (ctx, x, y);
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
							vkvg_rel_line_to (ctx, x, y);
							repeat = parse_floats(tmp, 1, &x);
						} else
							parseError = true;
						break;
					case 'H':
						if (parse_floats(tmp, 1, &x)) {
							vkvg_get_current_point (ctx, &c1x, &c1y);
							vkvg_line_to (ctx, x, c1y);
						} else
							parseError = true;
						break;
					case 'h':
						if (parse_floats(tmp, 1, &x))
							vkvg_rel_line_to (ctx, x, 0);
						else
							parseError = true;
						break;
					case 'V':
						if (parse_floats(tmp, 1, &y)) {
							vkvg_get_current_point (ctx, &c1x, &c1y);
							vkvg_line_to (ctx, c1x, y);
						} else
							parseError = true;
						break;
					case 'v':
						if (parse_floats(tmp, 1, &y))
							vkvg_rel_line_to (ctx, 0, y);
						else
							parseError = true;
						break;
					case 'Q':
						if (repeat)
							result = parse_floats(tmp, 3, &c1y, &x, &y);
						else
							result = parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
						if (result) {
							vkvg_quadratic_to (ctx, c1x, c1y, x, y);
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
							vkvg_get_current_point(ctx, &cpX, &cpY);
							vkvg_rel_quadratic_to (ctx, c1x, c1y, x, y);
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
							vkvg_get_current_point(ctx, &cpX, &cpY);
							if (prev == quad) {
								c1x = 2.0 * cpX - c1x;
								c1y = 2.0 * cpY - c1y;
							} else {
								c1x = x;
								c1y = y;
							}
							vkvg_quadratic_to (ctx, c1x, c1y, x, y);
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
							vkvg_get_current_point(ctx, &cpX, &cpY);
							if (prev == quad) {
								c1x = (cpX-c1x);
								c1y = (cpY-c1y);
							} else {
								c1x = x;
								c1y = y;
							}
							vkvg_rel_quadratic_to (ctx, c1x, c1y, x, y);
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
							vkvg_curve_to (ctx, c1x, c1y, c2x, c2y, x, y);
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
							vkvg_get_current_point(ctx, &cpX, &cpY);
							vkvg_rel_curve_to (ctx, c1x, c1y, c2x, c2y, x, y);
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
							vkvg_get_current_point(ctx, &cpX, &cpY);
							if (prev == cubic) {
								vkvg_curve_to (ctx, 2.0 * cpX - c2x, 2.0 * cpY - c2y, c1x, c1y, x, y);
							} else
								vkvg_curve_to (ctx, cpX, cpY, c1x, c1y, x, y);

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
							vkvg_get_current_point(ctx, &cpX, &cpY);
							if (prev == cubic) {
								vkvg_rel_curve_to (ctx, cpX - c2x, cpY - c2y, c1x, c1y, x, y);
							} else
								vkvg_rel_curve_to (ctx, 0, 0, c1x, c1y, x, y);

							c2x = cpX + c1x;
							c2y = cpY + c1y;
							prev = cubic;
							repeat = parse_floats(tmp, 1, &c1x);
							continue;
						} else
							parseError = true;
						break;
					case 'z':
					case 'Z':
						vkvg_close_path(ctx);
						break;
					}
					prev = none;
				}
				fclose (tmp);
			} else
				printf("Unprocessed attribute: %s->%s\n", elt, att);
		}
		res = get_attribute;
	}
	if (hasFill || hasStroke)
		draw (hasStroke, hasFill, stroke, fill);
	read_tag_end
	return res;
}

int read_g_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			printf("Unprocessed attribute: %s->%s\n", elt, att);
		}
		res = get_attribute;
	}
	read_tag_end
	return res;
}

int read_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			printf("Unprocessed attribute: %s->%s\n", elt, att);
		}
		res = get_attribute;
	}
	read_tag_end
}
int read_tag (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {

	while (!feof (f)) {
		res = fscanf(f, " <%[^> ]", elt);
		if (res < 0)
			return 0;
		if (!res) {
			res = fscanf(f, "%*[^<]<%[^> ]", elt);
			if (!res) {
				printf("element name parsing error (%s)\n", elt);
				return -1;
			}
		}
		if (elt[0] == '/') {
			if (getc(f) != '>') {
				printf("parsing error, expecting '>'\n");
				return -1;
			}
			//read_attributes_ptr = NULL;
			return 0;
		}
		if (elt[0] == '!') {
			if (!strcmp(elt,"!DOCTYPE")) {
				while (!feof (f))
					if (fgetc(f)=='>')
						break;;
			} if (!strncmp(elt, "!--", 3)) {
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
		if (!strcmp(elt,"svg"))
			res = read_svg_attributes	(f, hasStroke, hasFill, stroke, fill);
		else if (!strcmp(elt,"g")) {
			vkvg_save (ctx);
			res = read_g_attributes		(f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (ctx);
		} else if (!strcmp(elt,"rect")) {
			vkvg_save (ctx);
			res = read_rect_attributes	(f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (ctx);
		} else if (!strcmp(elt,"line")) {
			vkvg_save (ctx);
			res = read_line_attributes	(f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (ctx);
		} else if (!strcmp(elt,"circle")) {
			vkvg_save (ctx);
			res = read_circle_attributes (f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (ctx);
		} else if (!strcmp(elt,"path")) {
			vkvg_save (ctx);
			res = read_path_attributes	(f, hasStroke, hasFill, stroke, fill);
			vkvg_restore (ctx);
		} else
			res = read_attributes		(f, hasStroke, hasFill, stroke, fill);
	}
	return res;
}


static VkSampleCountFlags samples = VK_SAMPLE_COUNT_8_BIT;
static uint32_t width=512, height=512;
static bool paused = false;

char filename[256];
char* directory;

struct stat file_stat;

#define NORMAL_COLOR  "\x1B[0m"
#define GREEN  "\x1B[32m"
#define BLUE  "\x1B[34m"

void show_dir_content(char * path)
{
  DIR * d = opendir(path); // open the path
  if(d==NULL) return; // if was not able, return
  struct dirent * dir; // for the directory entries
  while ((dir = readdir(d)) != NULL) // if we were able to read somehting from the directory
	{
	  if(dir-> d_type != DT_DIR) // if the type is not directory just print it with blue color
		printf("%s%s\n",BLUE, dir->d_name);
	  else
	  if(dir -> d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ) // if it is a directory
	  {
		printf("%s%s\n",GREEN, dir->d_name); // print its name in green
		char d_path[255]; // here I am using sprintf which is safer than strcat
		sprintf(d_path, "%s/%s", path, dir->d_name);
		show_dir_content(d_path); // recall with the new path
	  }
	}
	closedir(d); // finally close the directory
}

void readSVG (VkEngine e) {
	struct stat sb;
	if (stat(filename, &sb) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
	if (svgSurf && sb.st_mtim.tv_sec == file_stat.st_mtim.tv_sec)
		return;

	file_stat = sb;

	FILE* f = fopen(filename, "r");
	if (f == NULL){
		printf("error: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	vkengine_wait_idle(e);

	vkengine_set_title(e, filename);

	read_tag (f, false, true, 0xffffffff, 0xffffffff);

	fclose(f);

	VkvgSurface tmp = svgSurf;
	svgSurf = newSvgSurf;
	newSvgSurf = NULL;
	if (tmp)
		vkvg_surface_destroy(tmp);
	vkengine_wait_idle(e);
}
DIR * pCurrentDir;
void get_next_svg_file () {
	struct dirent *dir;
	while ((dir = readdir(pCurrentDir)) != NULL) {
		if(dir->d_type != DT_DIR) {
			if (!strcasecmp(strrchr(dir->d_name, '\0') - 4, ".svg")){
				sprintf(filename, "%s/%s", directory, dir->d_name);
				return ;
			}
		}
	}
	closedir(pCurrentDir);
	if (!filename[0]) {
		printf ("No svg file found in %s\n", directory);
		exit(0);
	}
	pCurrentDir = opendir(directory);

	get_next_svg_file();
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_RELEASE)
		return;
	switch (key) {
	case GLFW_KEY_SPACE:
		 paused = !paused;
		break;
	case GLFW_KEY_ESCAPE :
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		break;
	case GLFW_KEY_ENTER :
		get_next_svg_file();
		file_stat = (struct stat){0};
		break;
	case GLFW_KEY_KP_ADD:
		scale*=2.0;
		file_stat = (struct stat){0};
		break;
	case GLFW_KEY_KP_SUBTRACT:
		scale/=2.0;
		file_stat = (struct stat){0};
		break;
	}
}


int main (int argc, char *argv[]){

	if (argc > 1)
		directory = argv[1];
	else
		directory = "images";

	pCurrentDir = opendir(directory); // open the path
	if(pCurrentDir==NULL) return -1; // if was not able, return
	filename[0] = 0;
	get_next_svg_file();

	VkEngine e = vkengine_create (VK_PRESENT_MODE_FIFO_KHR, width, height);
	vkengine_set_key_callback (e, key_callback);

	dev = vkvg_device_create_multisample(vkh_app_get_inst(e->app),
			 vkengine_get_physical_device(e), vkengine_get_device(e), vkengine_get_queue_fam_idx(e), 0, samples, false);

	VkvgSurface surf = vkvg_surface_create(dev, width, height);

	vkh_presenter_build_blit_cmd (e->renderer, vkvg_surface_get_vk_image(surf), width, height);


	while (!vkengine_should_close (e)) {

		readSVG (e);

		glfwPollEvents();

		if (!vkh_presenter_draw (e->renderer)){
			vkh_presenter_get_size (e->renderer, &width, &height);
			vkvg_surface_destroy (surf);
			surf = vkvg_surface_create(dev, width, height);
			vkh_presenter_build_blit_cmd (e->renderer, vkvg_surface_get_vk_image(surf), width, height);
			vkengine_wait_idle(e);
			continue;
		}
		ctx = vkvg_create(surf);
		vkvg_set_source_rgb(ctx,0.1,0.1,0.1);
		vkvg_paint(ctx);

		if (svgSurf) {
			vkvg_set_source_surface(ctx, svgSurf, 10, 10);
			vkvg_paint(ctx);
		} else {
			vkvg_set_line_width(ctx,10);
			vkvg_set_source_rgb(ctx,1,0,0);
			vkvg_move_to(ctx, 0,0);
			vkvg_line_to(ctx, 512,512);
			vkvg_move_to(ctx, 0,512);
			vkvg_line_to(ctx, 512,0);
			vkvg_stroke(ctx);

		}
		vkvg_destroy(ctx);
	}
	if (svgSurf)
		vkvg_surface_destroy(svgSurf);
	vkvg_surface_destroy(surf);
	vkvg_device_destroy(dev);
	vkengine_destroy(e);

	closedir(pCurrentDir);
}
