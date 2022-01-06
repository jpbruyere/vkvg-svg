// Copyright (c) 2022  Jean-Philippe Bruyère <jp_bruyere@hotmail.com>
//
// This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

#include "vkvg-svg.h"

#define ARRAY_INIT	8
#define ARRAY_ELEMENT_TYPE void*

#define ARRAY_IMPLEMENTATION
#include "array.h"


#include "vkvg_svg_internal.h"

#define CREATE_CTOR_ELT(elt) \
	svg_element_##elt* _new_##elt() {\
		svg_element_##elt* c = (svg_element_##elt*)calloc (1, sizeof (svg_element_##elt));\
		c->id.type = svg_element_type_##elt;\
		return c;\
	}

CREATE_CTOR_ELT(rect)
CREATE_CTOR_ELT(circle)
CREATE_CTOR_ELT(line)
CREATE_CTOR_ELT(ellipse)
CREATE_CTOR_ELT(path)

#define CASTELT(var,type,data) svg_element_##type* var = (svg_element_##type*)data



svg_element_linear_gradient* _new_linear_gradient() {
	svg_element_linear_gradient* g = (svg_element_linear_gradient*)calloc(1,sizeof(svg_element_linear_gradient));
	g->id.type = svg_element_type_linear_gradient;
	g->x1 = (svg_length_or_percentage){0,svg_unit_percentage};
	g->y1 = (svg_length_or_percentage){0,svg_unit_percentage};
	g->x2 = (svg_length_or_percentage){100,svg_unit_percentage};
	g->y2 = (svg_length_or_percentage){0,svg_unit_percentage};
	return g;
}
svg_element_radial_gradient* _new_radial_gradient() {
	svg_element_radial_gradient* g = (svg_element_radial_gradient*)calloc(1,sizeof(svg_element_radial_gradient));\
	g->id.type = svg_element_type_radial_gradient;\
	g->gradientUnits = svg_gradient_unit_objectBoundingBox;\
	g->cx = (svg_length_or_percentage){50,svg_unit_percentage};\
	g->cy = (svg_length_or_percentage){50,svg_unit_percentage};\
	g->fx = (svg_length_or_percentage){0};\
	g->fy = (svg_length_or_percentage){0};\
	g->r = (svg_length_or_percentage){50,svg_unit_percentage};
	return g;
}
uint32_t _get_element_hash (void* elt) {
	return ((svg_element_id*)elt)->hash;
}
svg_element_type _get_element_type (void* elt) {
	return ((svg_element_id*)elt)->type;
}

uint32_t hash_string(const char * s)
{
	uint32_t hash = 0;

	for(; *s; ++s)
	{
		hash += *s;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

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
		case 'r':
			return 0x00000000;//Transparent
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
bool try_parse_lenghtOrPercentage (const char* measure, float* number, svg_unit* units) {
	char str_units[16];
	int res = sscanf(measure, "%f%[^, ]", number, str_units);
	if (res == 0)
		return false;
	if (res == 2) {
		int l = strlen(str_units);
		if (l == 1) {
			switch (tolower(str_units[0])) {
			case 'q':
				*units = svg_unit_Q;
				return true;
			case '%':
				*units = svg_unit_percentage;
				return true;
			default:
				LOG ("error parsing measure: %s\n", measure);
				return false;
			}
		}
		if (l == 2) {
			switch (tolower(str_units[0])) {
			case 'd':
				*units = svg_unit_deg;
				return true;
				break;
			case 'g':
				*units = svg_unit_grad;
				return true;
				break;
			case 'r':
				*units = svg_unit_rad;
				return true;
				break;
			case 'p':
				switch (tolower(str_units[1])) {
				case 't':
					*units = svg_unit_pt;
					return true;
				case 'c':
					*units = svg_unit_pc;
					return true;
				case 'x':
					*units = svg_unit_px;
					return true;
				default:
					LOG ("error parsing measure: %s\n", measure);
				}
				break;
			case 'i':
				if (tolower(str_units[1]) != 'n') {
					LOG ("error parsing measure: %s\n", measure);
					return false;
				}
				*units = svg_unit_in;
				return true;
				break;
			case 'm':
				if (tolower(str_units[1]) != 'm') {
					LOG ("error parsing measure: %s\n", measure);
					return false;
				}
				*units = svg_unit_mm;
				return true;
				break;
			case 'c':
				if (tolower(str_units[1]) != 'm') {
					LOG ("error parsing measure: %s\n", measure);
					return false;
				}
				*units = svg_unit_cm;
				return true;
				break;
			default:
				LOG ("error parsing measure: %s\n", measure);
				return false;
			}

		}
	}
	return true;
}

bool try_parse_lenghtOrPercentage2 (const char* measure, svg_length_or_percentage* lop) {
	char str_units[16];
	int res = sscanf(measure, "%f%[^, ]", &lop->number, str_units);
	if (res == 0)
		return false;
	if (res == 2) {
		int l = strlen(str_units);
		if (l == 1) {
			switch (tolower(str_units[0])) {
			case 'q':
				lop->units = svg_unit_Q;
				return true;
			case '%':
				lop->units = svg_unit_percentage;
				return true;
			default:
				LOG ("error parsing measure: %s\n", measure);
				return false;
			}
		}
		if (l == 2) {
			switch (tolower(str_units[0])) {
			case 'p':
				switch (tolower(str_units[1])) {
				case 't':
					lop->units = svg_unit_pt;
					return true;
				case 'c':
					lop->units = svg_unit_pc;
					return true;
				case 'x':
					lop->units = svg_unit_px;
					return true;
				default:
					LOG ("error parsing measure: %s\n", measure);
				}
				break;
			case 'i':
				if (tolower(str_units[1]) != 'n') {
					LOG ("error parsing measure: %s\n", measure);
					return false;
				}
				lop->units = svg_unit_in;
				return true;
				break;
			case 'm':
				if (tolower(str_units[1]) != 'm') {
					LOG ("error parsing measure: %s\n", measure);
					return false;
				}
				lop->units = svg_unit_mm;
				return true;
				break;
			case 'c':
				if (tolower(str_units[1]) != 'm') {
					LOG ("error parsing measure: %s\n", measure);
					return false;
				}
				lop->units = svg_unit_cm;
				return true;
				break;
			default:
				LOG ("error parsing measure: %s\n", measure);
				return false;
			}

		}
	}
	return true;
}
bool try_parse_color (const char* colorString, svg_paint_type* isEnabled, uint32_t* colorValue) {
	if (colorString[0] == '#') {
		char color[7];
		int res = sscanf(&colorString[1], " %[0-9A-Fa-f]", color);
		char hexColorString[12];
		if (strlen(color) == 3)
			sprintf(hexColorString, "0xff%c%c%c%c%c%c", color[2],color[2],color[1],color[1],color[0],color[0]);
		else
			sprintf(hexColorString, "0xff%c%c%c%c%c%c", color[4],color[5],color[2],color[3],color[0],color[1]);
		sscanf(hexColorString, "%x", colorValue);
		*isEnabled = svg_paint_type_solid;
		return true;
	}
	if (!strncmp (colorString, "url", 3)) {
		char iri[128];
		if (sscanf(&colorString[3], " (%[^)])", iri)) {
			if (iri[0] == '#') {
				*colorValue = hash_string(&iri[1]);
				*isEnabled = svg_paint_type_pattern;
				return true;
			}
		}
		LOG ("error parsing uri: %s\n", colorString);
		return false;
	}
	uint32_t r = 0, g = 0, b = 0, a = 0;
	if (sscanf(colorString, "rgb ( %d%*[ ,]%d%*[ ,]%d )", &r, &g, &b))
		*colorValue = 0xff000000 + (b << 16) + (g << 8) + r;
	else if (sscanf(colorString, "rgba ( %d%*[ ,]%d%*[ ,]%d%*[ ,]%d )", &r, &g, &b, &a))
		*colorValue = (a << 24) + (b << 16) + (g << 8) + r;
	else if (!strcasecmp (colorString, "none"))
		*colorValue = 0;
	else if (strcasecmp (colorString, "currentColor"))
		*colorValue = parseColorName(colorString);
	if (*colorValue > 0)
		*isEnabled = svg_paint_type_solid;
	else
		*isEnabled = svg_paint_type_none;
	return true;
}
void _parse_lenghtOrPercentage (svg_context* svg, svg_length_or_percentage* lop) {
	svg_length_or_percentage result;
	if (try_parse_lenghtOrPercentage2(svg->value, &result))
		*lop = result;
	else
		LOG("error parsing attribute: %s->%s=%s\n", svg->elt, svg->att, svg->value);
}
float _parse_opacity (svg_context* svg) {
	svg_length_or_percentage opacity;
	if (!try_parse_lenghtOrPercentage2(svg->value, &opacity)) {
		LOG ("error parsing gradient opacity: %s\n", svg->value);
		return 1;
	}
	if (opacity.units == svg_unit_percentage)
		return opacity.number / 100.0f;
	else
		return opacity.number;
}

bool _try_parse_transform (svg_context* svg, vkvg_matrix_t* mat) {
	FILE *tmp = fmemopen(svg->value, strlen (svg->value), "r");
	char transform[16];
	while (!feof(tmp)) {
		if (fscanf(tmp, " %[^(](", transform) == 1) {
			if (!strcasecmp (transform, "none"))
				break;
			else if (!strcasecmp (transform, "matrix")) {
				vkvg_matrix_t m, newMat;
				if (!parse_floats(tmp, 6, &m.xx, &m.yx, &m.xy, &m.yy, &m.x0, &m.y0)) {
					LOG ("error parsing transformation matrix: %s\n", svg->value);
					fclose (tmp);
					return false;
				}

				vkvg_matrix_multiply(&newMat, &m, mat);
				*mat = newMat;
			} else if (!strcasecmp (transform, "translate")) {
				float dx, dy;
				if (!parse_floats(tmp, 1, &dx)) {
					LOG ("error parsing translation transform: %s\n", svg->value);
					fclose (tmp);
					return false;
				}
				if (parse_floats(tmp, 1, &dy))
					vkvg_matrix_translate(mat, dx, dy);
				else
					vkvg_matrix_translate(mat, dx, 0);
			} else if (!strcasecmp (transform, "scale")) {
				float sx, sy;
				if (!parse_floats(tmp, 1, &sx)) {
					LOG ("error parsing scale transform: %s\n", svg->value);
					fclose (tmp);
					return false;
				}
				if (parse_floats(tmp, 1, &sy))
					vkvg_matrix_scale(mat, sx, sy);
				else
					vkvg_matrix_scale(mat, sx, sx);
			} else if (!strcasecmp (transform, "rotate")) {
				float angle, cx, cy;
				if (!parse_floats(tmp, 1, &angle)) {
					LOG ("error parsing rotate transform: %s\n", svg->value);
					fclose (tmp);
					return false;
				}
				if (parse_floats(tmp, 2, &cx, &cy)) {
					vkvg_matrix_translate	(mat, cx, cy);
					vkvg_matrix_rotate		(mat, degToRad(angle));
					vkvg_matrix_translate	(mat, cx, cy);
				} else
					vkvg_matrix_rotate (mat, degToRad(angle));
			} else {
				LOG ("unimplemented transform: %s\n", transform);
				fclose (tmp);
				return false;
			}
			fscanf(tmp, "[^)]");
			if (getc(tmp) != ')') {
				LOG ("error parsing transformation, expecting ')'\n");
				fclose (tmp);
				return false;
			}
		} else if (feof(tmp))
			break;
	}

	fclose (tmp);
	return true;
}
void _apply_transform (svg_context* svg) {
	vkvg_matrix_t current;
	vkvg_get_matrix(svg->ctx, &current);
	if (_try_parse_transform(svg, &current))
		vkvg_set_matrix(svg->ctx, &current);
}
void _parse_point_list (svg_context* svg) {
	float x = 0, y = 0;
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
}

typedef struct {
	char* str;
	int pos;
	int len;
}_stream_t;

typedef _stream_t* stream;

#define NEW_STREAM(var, str) \
	_stream_t streamvar_##var = {str,0,strlen(str)};\
	stream var = &streamvar_##var

#define STR_EOF(s) (s->pos >= s->len)
#define STR_PEEK(s) s->str[s->pos]
#define STR_GET(s) s->str[s->pos++]
#define STR_ADVANCE(s) s->pos++
#define STR_SKIP(s,c) if(!STR_EOF(s) && s->str[s->pos]==c) s->pos++

#define ISWHITESPACE(c) (c==0x9 || c==0x20 || c==0xA || c==0xD)

void stream_skip_white_spaces (stream s) {
	while (!STR_EOF(s) && ISWHITESPACE(STR_PEEK(s)))
		s->pos++;
}
bool _try_parse_float (stream s, float* f) {
	int start = s->pos;
	bool hasDecimal = false, hasDigit = false, hasExponent=false;
	while(!STR_EOF(s)) {
		if (STR_PEEK(s)=='0') {
			STR_ADVANCE(s);
			if (!hasDecimal && !hasDigit && !STR_EOF(s) && STR_PEEK(s)=='0')
				break;
			hasDigit = true;
		} else if (STR_PEEK(s) >= '0' && STR_PEEK(s) <= '9') {
			STR_ADVANCE(s);
			hasDigit = true;
		} else if (STR_PEEK(s)=='-' || STR_PEEK(s)=='+') {
			if (s->pos - start > 0)
				break;
			STR_ADVANCE(s);
		} else if (STR_PEEK(s)=='e' || STR_PEEK(s)=='E') {
			STR_ADVANCE(s);
			if (!STR_EOF(s) && (STR_PEEK(s)=='-' || STR_PEEK(s)=='+')){
				STR_ADVANCE(s);
				hasExponent = true;
			} else
				return false;
		} else if (STR_PEEK(s)=='.') {
			if (hasDecimal||hasExponent)
				break;
			hasDecimal = true;
			STR_ADVANCE(s);
		} else
			break;
	}
	if (hasDigit) {
		char* pEnd = &s->str[s->pos];
		if (STR_EOF(s))
			*f = strtod (&s->str[start], NULL);
		else
			*f = strtod (&s->str[start], &pEnd);
		return true;
	}
	return false;
}
bool _try_parse_floats (stream s, int floatCount, ...) {
	va_list args;
	va_start(args, floatCount);

	for (int i=0; i<floatCount; i++) {
		float* pF = va_arg(args, float*);

		stream_skip_white_spaces (s);
		if (!_try_parse_float(s, pF)) {
			va_end(args);
			return false;
		}
		stream_skip_white_spaces (s);
		STR_SKIP(s,',');
	}
	va_end(args);
	return true;
}

void _parse_path_d_attribute (svg_context* svg, char* str) {
	if (!str)
		return;
	float x, y, c1x, c1y, c2x, c2y, cpX, cpY, rx, ry, rotx, large, sweep;
	enum prevCmd prev = none;
	int repeat=0;
	float subpathX = 0, subpathY = 0;
	char c;
	NEW_STREAM(tmp,str);
	bool result = false;
	while (!STR_EOF(tmp)) {

		stream_skip_white_spaces (tmp);

		if (!repeat)
			c = STR_GET(tmp);

		switch (c) {
		case 'M':
			if (repeat)
				result = _try_parse_floats(tmp, 1, &y);
			else
				result = _try_parse_floats(tmp, 2, &x, &y);
			if (result) {
				if (repeat)
					vkvg_line_to (svg->ctx, x, y);
				else {
					vkvg_move_to (svg->ctx, x, y);
					vkvg_get_current_point(svg->ctx, &subpathX, &subpathY);
				}
				repeat = _try_parse_floats(tmp, 1, &x);
			} else
				return;
			break;
		case 'm':
			if (repeat)
				result = _try_parse_floats(tmp, 1, &y);
			else
				result = _try_parse_floats(tmp, 2, &x, &y);
			if (result) {
				if (repeat)
					vkvg_rel_line_to (svg->ctx, x, y);
				else {
					vkvg_rel_move_to (svg->ctx, x, y);
					vkvg_get_current_point(svg->ctx, &subpathX, &subpathY);
				}
				repeat = _try_parse_floats(tmp, 1, &x);
			} else
				return;
			break;
		case 'L':
			if (repeat)
				result = _try_parse_floats(tmp, 1, &y);
			else
				result = _try_parse_floats(tmp, 2, &x, &y);
			if (result) {
				vkvg_line_to (svg->ctx, x, y);
				repeat = _try_parse_floats(tmp, 1, &x);
			} else
				return;
			break;
		case 'l':
			if (repeat)
				result = _try_parse_floats(tmp, 1, &y);
			else
				result = _try_parse_floats(tmp, 2, &x, &y);
			if (result) {
				vkvg_rel_line_to (svg->ctx, x, y);
				repeat = _try_parse_floats(tmp, 1, &x);
			} else
				return;
			break;
		case 'H':
			if (!repeat) {
				if (!_try_parse_floats(tmp, 1, &x))
					return;
			}
			vkvg_get_current_point (svg->ctx, &c1x, &c1y);
			vkvg_line_to (svg->ctx, x, c1y);
			repeat = _try_parse_floats(tmp, 1, &x);
			break;
		case 'h':
			if (!repeat) {
				if (!_try_parse_floats(tmp, 1, &x))
					return;
			}
			vkvg_rel_line_to (svg->ctx, x, 0);
			repeat = _try_parse_floats(tmp, 1, &x);
			break;
		case 'V':
			if (!repeat) {
				if (!_try_parse_floats(tmp, 1, &y))
					return;
			}
			vkvg_get_current_point (svg->ctx, &c1x, &c1y);
			vkvg_line_to (svg->ctx, c1x, y);
			repeat = _try_parse_floats(tmp, 1, &y);
			break;
		case 'v':
			if (!repeat) {
				if (!_try_parse_floats(tmp, 1, &y))
					return;
			}
			vkvg_rel_line_to (svg->ctx, 0, y);
			repeat = _try_parse_floats(tmp, 1, &y);
			break;
		case 'Q':
			if (repeat)
				result = _try_parse_floats(tmp, 3, &c1y, &x, &y);
			else
				result = _try_parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
			if (result) {
				vkvg_quadratic_to (svg->ctx, c1x, c1y, x, y);
				prev = quad;
				repeat = _try_parse_floats(tmp, 1, &c1x);
				continue;
			} else
				return;
			break;
		case 'q':
			if (repeat)
				result = _try_parse_floats(tmp, 3, &c1y, &x, &y);
			else
				result = _try_parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
			if (result) {
				vkvg_get_current_point(svg->ctx, &cpX, &cpY);
				vkvg_rel_quadratic_to (svg->ctx, c1x, c1y, x, y);
				prev = quad;
				c1x += cpX;
				c1y += cpY;
				repeat = _try_parse_floats(tmp, 1, &c1x);
				continue;
			} else
				return;
			break;
		case 'T':
			if (repeat)
				result = _try_parse_floats(tmp, 1, &y);
			else
				result = _try_parse_floats(tmp, 2, &x, &y);
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
				repeat = _try_parse_floats(tmp, 1, &x);
				continue;
			} else
				return;
			break;
		case 't':
			if (repeat)
				result = _try_parse_floats(tmp, 1, &y);
			else
				result = _try_parse_floats(tmp, 2, &x, &y);
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
				repeat = _try_parse_floats(tmp, 1, &x);
				continue;
			} else
				return;
			break;
		case 'C':
			if (repeat)
				result = _try_parse_floats(tmp, 5, &c1y, &c2x, &c2y, &x, &y);
			else
				result = _try_parse_floats(tmp, 6, &c1x, &c1y, &c2x, &c2y, &x, &y);
			if (result) {
				vkvg_curve_to (svg->ctx, c1x, c1y, c2x, c2y, x, y);
				prev = cubic;
				repeat = _try_parse_floats(tmp, 1, &c1x);
				continue;
			} else
				return;
			break;
		case 'c':
			if (repeat)
				result = _try_parse_floats(tmp, 5, &c1y, &c2x, &c2y, &x, &y);
			else
				result = _try_parse_floats(tmp, 6, &c1x, &c1y, &c2x, &c2y, &x, &y);
			if (result) {
				vkvg_get_current_point(svg->ctx, &cpX, &cpY);
				vkvg_rel_curve_to (svg->ctx, c1x, c1y, c2x, c2y, x, y);
				c2x += cpX;
				c2y += cpY;
				prev = cubic;
				repeat = _try_parse_floats(tmp, 1, &c1x);
				continue;
			} else
				return;
			break;
		case 'S':
			if (repeat)
				result = _try_parse_floats(tmp, 3, &c1y, &x, &y);
			else
				result = _try_parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
			if (result) {
				vkvg_get_current_point(svg->ctx, &cpX, &cpY);
				if (prev == cubic) {
					vkvg_curve_to (svg->ctx, 2.0 * cpX - c2x, 2.0 * cpY - c2y, c1x, c1y, x, y);
				} else
					vkvg_curve_to (svg->ctx, cpX, cpY, c1x, c1y, x, y);

				c2x = c1x;
				c2y = c1y;
				prev = cubic;
				repeat = _try_parse_floats(tmp, 1, &c1x);
				continue;
			} else
				return;
			break;
		case 's':
			if (repeat)
				result = _try_parse_floats(tmp, 3, &c1y, &x, &y);
			else
				result = _try_parse_floats(tmp, 4, &c1x, &c1y, &x, &y);
			if (result) {
				vkvg_get_current_point(svg->ctx, &cpX, &cpY);
				if (prev == cubic) {
					vkvg_rel_curve_to (svg->ctx, cpX - c2x, cpY - c2y, c1x, c1y, x, y);
				} else
					vkvg_rel_curve_to (svg->ctx, 0, 0, c1x, c1y, x, y);

				c2x = cpX + c1x;
				c2y = cpY + c1y;
				prev = cubic;
				repeat = _try_parse_floats(tmp, 1, &c1x);
				continue;
			} else
				return;
			break;
		case 'A'://rx ry x-axis-rotation large-arc-flag sweep-flag x y
			if (repeat)
				result = _try_parse_floats(tmp, 6, &ry, &rotx, &large, &sweep, &x, &y);
			else
				result = _try_parse_floats(tmp, 7, &rx, &ry, &rotx, &large, &sweep, &x, &y);
			if (result) {
				bool lf = large > __FLT_EPSILON__;
				bool sw = sweep > __FLT_EPSILON__;
				rotx = rotx * M_PI / 180.0f;

				vkvg_elliptic_arc (svg->ctx, x, y, lf, sw, rx, ry, rotx);

				repeat = _try_parse_floats(tmp, 1, &rx);
			} else
				return;
			break;
		case 'a'://rx ry x-axis-rotation large-arc-flag sweep-flag x y
			if (repeat)
				result = _try_parse_floats(tmp, 6, &ry, &rotx, &large, &sweep, &x, &y);
			else
				result = _try_parse_floats(tmp, 7, &rx, &ry, &rotx, &large, &sweep, &x, &y);
			if (result) {
				vkvg_get_current_point(svg->ctx, &cpX, &cpY);
				bool lf = large > __FLT_EPSILON__;
				bool sw = sweep > __FLT_EPSILON__;
				rotx = degToRad (rotx);

				vkvg_elliptic_arc(svg->ctx, cpX + x, cpY + y, lf, sw, rx, ry, rotx);

				repeat = _try_parse_floats(tmp, 1, &rx);
			} else
				return;
			break;
		case 'z':
		case 'Z':
			vkvg_close_path(svg->ctx);
			vkvg_move_to(svg->ctx, subpathX, subpathY);
			break;
		default:
			LOG ("error parsing path: unexpected char: %c\n", c);
			return;
		}
		prev = none;
	}

}
bool try_find_by_id (svg_context* svg, uint32_t hash, void** elt) {
	*elt = NULL;
	for (uint32_t i=0; i < svg->idList->count; i++) {
		if (_get_element_hash (svg->idList->elements[i]) == hash) {
			*elt = svg->idList->elements[i];
			return true;
		}
	}
	return false;
}
bool _idlist_contains (svg_context* svg, uint32_t hash) {
	for (uint32_t i=0; i < svg->idList->count; i++) {
		if (_get_element_hash (svg->idList->elements[i]) == hash)
			return true;
	}
	return false;
}
void _store_or_throw (svg_context* svg, void* elt) {
	if (svg->currentIdHash) {
		svg_element_id* id = (svg_element_id*)elt;
		id->hash = svg->currentIdHash;
		LOG("store elemnt: %u type:%u\n", id->hash, id->type);
		array_add (svg->idList, elt);
	} else
		free (elt);
}
float _get_pixel_coord (float reference, svg_length_or_percentage* lop) {
	switch (lop->units) {
	case svg_unit_percentage:
		 return reference * lop->number / 100.0f;
	default:
		return lop->number;
	}
}
void _copy_pattern_color_stops (VkvgPattern orig, VkvgPattern dest) {
	uint32_t stopCount;
	if (vkvg_pattern_get_color_stop_count(orig, &stopCount) == VKVG_STATUS_SUCCESS) {
		for (uint32_t i=0; i<stopCount; i++) {
			float offset, r, g, b, a;
			vkvg_pattern_get_color_stop_rgba(orig, i, &offset, &r, &g, &b, &a);
			vkvg_pattern_add_color_stop(dest, offset, r, g, b, a);
		}
	} else
		LOG ("Error processing referenced pattern\n");
}
void _resolve_pattern_href (svg_context* svg, void* rootElt, VkvgPattern pat) {
	void* elt = NULL;
	svg_element_id* id = (svg_element_id*)rootElt;
	while (id->xlinkHref) {
		if (try_find_by_id(svg, id->xlinkHref, &elt)) {
			id->xlinkHref = 0;//reset once resolved
			id = (svg_element_id*)elt;
		} else {
			LOG ("xlink:href svg element error  %s\n", svg->value);
			return;
		}
	}
	if (elt) {
		VkvgPattern refPatter = NULL;
		if (_get_element_type(elt) == svg_element_type_radial_gradient)
			refPatter = (VkvgPattern) ((svg_element_radial_gradient*)elt)->pattern;
		else if (_get_element_type(elt) == svg_element_type_linear_gradient)
			refPatter = (VkvgPattern) ((svg_element_linear_gradient*)elt)->pattern;
		else {
			LOG ("xlink:href svg element error, expecting gradient%s\n", svg->value);
			return;
		}
		_copy_pattern_color_stops(refPatter, pat);
	}
}
void set_pattern (svg_context* svg, uint32_t patternHash) {
	void* elt;
	VkvgPattern pat;
	if (try_find_by_id(svg, patternHash, &elt)) {
		switch (_get_element_type(elt)) {
		case svg_element_type_linear_gradient:
			{
				svg_element_linear_gradient* g = (svg_element_linear_gradient*)elt;

				_resolve_pattern_href (svg, elt, g->pattern);

				pat = g->pattern;

				float x0 = 0, y0 = 0, x1, y1;
				if (g->gradientUnits == svg_gradient_unit_objectBoundingBox)
					vkvg_path_extents(svg->ctx, &x0, &y0, &x1, &y1);
				else {
					x0 = svg->viewBox.x;
					y0 = svg->viewBox.y;
					x1 = svg->viewBox.x + svg->viewBox.w;
					y1 = svg->viewBox.y + svg->viewBox.h;
				}

				float w = x1 - x0;
				float h = y1 - y0;

				float px0,py0,px1,py1;
				px0 = _get_pixel_coord (w, &g->x1) + x0;
				py0 = _get_pixel_coord (h, &g->y1) + y0;
				px1 = _get_pixel_coord (w, &g->x2) + x0;
				py1 = _get_pixel_coord (h, &g->y2) + y0;

				vkvg_pattern_edit_linear(g->pattern, px0, py0, px1, py1);
				vkvg_set_source(svg->ctx, g->pattern);
			}

			break;
		case svg_element_type_radial_gradient:
			{
				svg_element_radial_gradient* g = (svg_element_radial_gradient*)elt;

				_resolve_pattern_href (svg, elt, g->pattern);

				pat = g->pattern;
				float x0 = 0, y0 = 0, x1, y1;
				if (g->gradientUnits == svg_gradient_unit_objectBoundingBox)
					vkvg_path_extents(svg->ctx, &x0, &y0, &x1, &y1);
				else {
					x0 = svg->viewBox.x;
					y0 = svg->viewBox.y;
					x1 = svg->viewBox.x + svg->viewBox.w;
					y1 = svg->viewBox.y + svg->viewBox.h;
				}

				float w = x1 - x0;
				float h = y1 - y0;

				float cx,cy,fx,fy,r;
				cx = _get_pixel_coord (w, &g->cx)+x0;
				cy = _get_pixel_coord (h, &g->cy)+y0;
				fx = _get_pixel_coord (w, &g->fx)+x0;
				fy = _get_pixel_coord (h, &g->fy)+y0;
				r = _get_pixel_coord (w, &g->r);

				vkvg_pattern_edit_radial (g->pattern, cx, cy, 0, cx, cy, r);
				vkvg_set_source(svg->ctx, g->pattern);
			}
			break;
		}
	} else
		LOG ("pattern hash not resolved: %d\n", patternHash);
}
int draw (svg_context* svg, svg_attributes* attribs) {
	if (attribs->hasFill) {
		vkvg_set_opacity(svg->ctx, attribs->opacity * attribs->fill_opacity);
		if (attribs->hasFill == svg_paint_type_pattern)
			set_pattern(svg, attribs->fill);
		else
			vkvg_set_source_color(svg->ctx, attribs->fill);
		if (attribs->hasStroke) {
			vkvg_fill_preserve(svg->ctx);
			vkvg_set_opacity(svg->ctx, attribs->opacity * attribs->stroke_opacity);
			if (attribs->hasStroke == svg_paint_type_pattern)
				set_pattern(svg, attribs->stroke);
			else
				vkvg_set_source_color(svg->ctx, attribs->stroke);
			vkvg_stroke(svg->ctx);
		} else
			vkvg_fill (svg->ctx);
	} else if (attribs->hasStroke) {
		vkvg_set_opacity(svg->ctx, attribs->opacity * attribs->fill_opacity);
		if (attribs->hasStroke == svg_paint_type_pattern)
			set_pattern(svg, attribs->stroke);
		else
			vkvg_set_source_color(svg->ctx, attribs->stroke);
		vkvg_stroke(svg->ctx);
	}
}
void _draw_text (svg_context* svg, FILE* f, svg_attributes* attribs,
				svg_length_or_percentage* x, svg_length_or_percentage* y, svg_length_or_percentage* dx, svg_length_or_percentage* dy) {
	if (attribs->hasFill) {
		if (attribs->hasFill == svg_paint_type_pattern)
			set_pattern(svg, attribs->fill);
		else
			vkvg_set_source_color(svg->ctx, attribs->fill);
	} else if (attribs->hasStroke) {
		if (attribs->hasStroke == svg_paint_type_pattern)
			set_pattern(svg, attribs->stroke);
		else
			vkvg_set_source_color(svg->ctx, attribs->stroke);
	}
	float cx = x->number + dx->number;
	float cy = y->number + dy->number;

	if (attribs->text_anchor > svg_text_anchor_start) {
		vkvg_text_extents_t e = {0};
		vkvg_text_extents(svg->ctx, svg->value, &e);
		if (attribs->text_anchor == svg_text_anchor_middle)
			cx -= e.width / 2.0f;
		else
			cx -= e.width;
	}

	vkvg_move_to(svg->ctx, cx, cy);
	vkvg_show_text(svg->ctx, svg->value);
}
float _normalized_diagonal (float w, float h) {
	return sqrtf(powf(w,2) + powf(h,2))/sqrtf(2);
}
void _process_element (svg_context* svg, svg_attributes* attribs, void* elt, bool use) {
	if (!svg->skipDraw) {
		switch (_get_element_type(elt)) {
		case svg_element_type_rect:
			{
				CASTELT(r,rect,elt);
				if (r->w.number && r->h.number && (attribs->hasFill || attribs->hasStroke)) {
					float	x = _get_pixel_coord(svg->viewBox.w, &r->x),
							y = _get_pixel_coord(svg->viewBox.h, &r->y),
							w = _get_pixel_coord(svg->viewBox.w, &r->w),
							h = _get_pixel_coord(svg->viewBox.h, &r->h),
							rx = _get_pixel_coord(svg->viewBox.w, &r->rx),
							ry = _get_pixel_coord(svg->viewBox.h, &r->ry);

					if (rx > w / 2.0f)
						rx = w / 2.0f;
					if (ry > h / 2.0f)
						ry = h / 2.0f;
					if (rx > 0 || ry > 0) {
						if (rx == 0)
							rx = ry;
						else if (ry == 0)
							ry = rx;
						vkvg_rounded_rectangle2(svg->ctx, x, y, w, h, rx, ry);
					} else
						vkvg_rectangle(svg->ctx, x, y, w, h);
					draw (svg, attribs);
				}
			}
			break;
		case svg_element_type_circle:
			{
				CASTELT(c,circle,elt);
				if (c->r.number > 0 && (attribs->hasFill || attribs->hasStroke)) {
					float	cx = _get_pixel_coord(svg->viewBox.w, &c->cx),
							cy = _get_pixel_coord(svg->viewBox.h, &c->cy),
							r  = _get_pixel_coord(_normalized_diagonal(svg->viewBox.w, svg->viewBox.h), &c->r);

					vkvg_arc(svg->ctx, cx, cy, r, 0, M_PI * 2);
					draw (svg, attribs);
				}
			}
			break;
		case svg_element_type_line:
			{
				CASTELT(l,line,elt);
				if (attribs->hasStroke) {
					float	x1 = _get_pixel_coord(svg->viewBox.w, &l->x1),
							y1 = _get_pixel_coord(svg->viewBox.h, &l->y1),
							x2 = _get_pixel_coord(svg->viewBox.w, &l->x2),
							y2 = _get_pixel_coord(svg->viewBox.h, &l->y2);

					vkvg_move_to(svg->ctx, x1, y1);
					vkvg_line_to(svg->ctx, x2, y2);

					attribs->hasFill = false;
					draw (svg, attribs);
				}
			}
			break;
		case svg_element_type_ellipse:
			{
				CASTELT(e,ellipse,elt);
				if (e->rx.number && e->ry.number && (attribs->hasFill || attribs->hasStroke)) {
					float	cx = _get_pixel_coord(svg->viewBox.w, &e->cx),
							cy = _get_pixel_coord(svg->viewBox.h, &e->cy),
							rx = _get_pixel_coord(svg->viewBox.w, &e->rx),
							ry = _get_pixel_coord(svg->viewBox.h, &e->ry);

					vkvg_ellipse (svg->ctx, rx, ry, cx, cy, 0);
					draw (svg, attribs);
				}
			}
			break;
		case svg_element_type_path:
			{
				CASTELT(p,path,elt);
				_parse_path_d_attribute (svg, p->d);
				draw (svg, attribs);
			}
			break;
		default:
			LOG ("Unprocessed element type: %d\n", _get_element_type(elt));
			return;
		}
	}
	if (!use)
		_store_or_throw(svg, elt);
}
void _process_use (svg_context* svg, svg_attributes* attribs) {
	if (!svg->currentXlinkHref) {
		LOG ("no xlink:href defined for use element\n");
		return;
	}
	void* elt;
	if (!try_find_by_id(svg, svg->currentXlinkHref, &elt)) {
		LOG ("xlink:href not resolved %s\n", svg->value);
		return;
	}
	_process_element(svg, attribs, elt, true);
}

#define PROCESS_SVG_XLINK_ATTRIB_XLINK_HREF \
{\
	if (svg->value[0] == '#') {\
		svg->currentXlinkHref = hash_string(&svg->value[1]);\
		LOG ("xlink:href %s -> %u\n", svg->value, svg->currentXlinkHref);\
	} else\
		LOG ("xlink:href type not handled %s\n", svg->value);\
}
#define PROCESS_SVG_XLINKEMBED_ATTRIB_XLINK_HREF PROCESS_SVG_XLINK_ATTRIB_XLINK_HREF

#define PROCESS_SVG_CORE_ATTRIB_ID svg->currentIdHash = hash_string(svg->value);

#define PROCESS_SVG_COLOR_ATTRIB_COLOR	try_parse_color (svg->value, &attribs.hasColor, &attribs.color);
#define PROCESS_SVG_PAINT_ATTRIB_STROKE	try_parse_color (svg->value, &attribs.hasStroke, &attribs.stroke);
#define PROCESS_SVG_PAINT_ATTRIB_FILL	try_parse_color (svg->value, &attribs.hasFill, &attribs.fill);
#define PROCESS_SVG_PAINT_ATTRIB_STROKE_WIDTH \
{\
	svg_unit units;\
	float value;\
	if (try_parse_lenghtOrPercentage(svg->value, &value, &units)) {\
		vkvg_set_line_width(svg->ctx, value);\
	}\
}
#define PROCESS_SVG_PAINT_ATTRIB_FILL_RULE_NONZERO		vkvg_set_fill_rule(svg->ctx, VKVG_FILL_RULE_NON_ZERO);
#define PROCESS_SVG_PAINT_ATTRIB_FILL_RULE_EVENODD		vkvg_set_fill_rule(svg->ctx, VKVG_FILL_RULE_EVEN_ODD);
#define PROCESS_SVG_PAINT_ATTRIB_STROKE_LINECAP_BUTT	vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_BUTT);
#define PROCESS_SVG_PAINT_ATTRIB_STROKE_LINECAP_ROUND	vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_ROUND);
#define PROCESS_SVG_PAINT_ATTRIB_STROKE_LINECAP_SQUARE	vkvg_set_line_cap(svg->ctx, VKVG_LINE_CAP_SQUARE);
#define PROCESS_SVG_PAINT_ATTRIB_STROKE_LINEJOIN_MITER	vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_MITER);
#define PROCESS_SVG_PAINT_ATTRIB_STROKE_LINEJOIN_ROUND	vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_ROUND);
#define PROCESS_SVG_PAINT_ATTRIB_STROKE_LINEJOIN_BEVEL	vkvg_set_line_join(svg->ctx, VKVG_LINE_JOIN_BEVEL);

#define HEADING_SVG_OPACITY_ATTRIB
#define PROCESS_SVG_OPACITY_ATTRIB
#define PROCESS_SVG_OPACITY_ATTRIB_OPACITY			attribs.opacity			= _parse_opacity (svg);
#define PROCESS_SVG_OPACITY_ATTRIB_FILL_OPACITY		attribs.fill_opacity	= _parse_opacity (svg);
#define PROCESS_SVG_OPACITY_ATTRIB_STROKE_OPACITY	attribs.stroke_opacity	= _parse_opacity (svg);


#define PROCESS_SVG_FONT_ATTRIB_FONT_FAMILY vkvg_select_font_face(svg->ctx, svg->value);
#define PROCESS_SVG_FONT_ATTRIB_FONT_SIZE {\
	svg_length_or_percentage font_size = {0, svg_unit_pt};\
	_parse_lenghtOrPercentage(svg, &font_size);\
	vkvg_set_font_size(svg->ctx, font_size.number);\
}

#define PROCESS_SVG_TEXTCONTENT_ATTRIB_TEXT_ANCHOR_START	attribs.text_anchor = svg_text_anchor_start;
#define PROCESS_SVG_TEXTCONTENT_ATTRIB_TEXT_ANCHOR_MIDDLE	attribs.text_anchor = svg_text_anchor_middle;
#define PROCESS_SVG_TEXTCONTENT_ATTRIB_TEXT_ANCHOR_END		attribs.text_anchor = svg_text_anchor_end;

#define PROCESS_G_TRANSFORM			_apply_transform (svg);
#define PROCESS_PATH_TRANSFORM		_apply_transform (svg);
#define PROCESS_RECT_TRANSFORM		_apply_transform (svg);
#define PROCESS_CIRCLE_TRANSFORM	_apply_transform (svg);
#define PROCESS_LINE_TRANSFORM		_apply_transform (svg);
#define PROCESS_ELLIPSE_TRANSFORM	_apply_transform (svg);
#define PROCESS_POLYLINE_TRANSFORM	_apply_transform (svg);
#define PROCESS_POLYGON_TRANSFORM	_apply_transform (svg);
#define PROCESS_TEXT_TRANSFORM		_apply_transform (svg);
#define PROCESS_USE_TRANSFORM		_apply_transform (svg);
#define PROCESS_DEFS_TRANSFORM		_apply_transform (svg);

//========SVG============
#define HEADING_SVG				\
	bool hasViewBox = false;	\
	svg_length_or_percentage width = {100.0f, svg_unit_percentage}, height = {100.0f, svg_unit_percentage};\
	svg_unit units;\
	float value;

#define ELEMENT_PRE_PROCESS_SVG \
	int surfW = 0, surfH = 0;\
	float xScale = 1, yScale = 1;\
	if (svg->fit) {\
		surfW = svg->width;\
		surfH = svg->height;\
	} else {\
		surfW = _get_pixel_coord(svg->width, &width);\
		surfH = _get_pixel_coord(svg->width, &height);\
	}\
	if (!hasViewBox) {\
		svg->viewBox = (svg_viewbox) {0,0,_get_pixel_coord(surfW, &width),_get_pixel_coord(surfH, &height)};\
	}\
	if (hasViewBox || svg->fit) {\
		if (surfW)\
			xScale = (float)surfW / svg->viewBox.w;\
		else\
			surfW = svg->viewBox.w;\
		if (surfH)\
			yScale = (float)surfH / svg->viewBox.h;\
		else\
			surfH = svg->viewBox.h;\
	}\
	svg->surf = vkvg_surface_create(svg->dev, surfW, surfH);\
	svg->ctx = vkvg_create (svg->surf);\
	vkvg_set_fill_rule(svg->ctx, VKVG_FILL_RULE_NON_ZERO);\
	if (xScale < yScale)\
		vkvg_scale(svg->ctx, xScale, xScale);\
	else\
		vkvg_scale(svg->ctx, yScale, yScale);\
	vkvg_clear(svg->ctx);

#define ELEMENT_POST_PROCESS_SVG vkvg_destroy(svg->ctx);

#define PROCESS_SVG_VIEWBOX \
{\
	char strX[64], strY[64], strW[64], strH[64];\
	res = sscanf(svg->value, " %s%*1[ ,]%s%*1[ ,]%s%*1[ ,]%s", strX, strY, strW, strH);\
	if (res==4) {\
		if (try_parse_lenghtOrPercentage(strX, &value, &units))\
			svg->viewBox.x = value;\
		else\
			LOG ("error parsing viewbox x: %s\n", strX);\
		if (try_parse_lenghtOrPercentage(strY, &value, &units))\
			svg->viewBox.y = value;\
		else\
			LOG ("error parsing viewbox y: %s\n", strY);\
		if (try_parse_lenghtOrPercentage(strW, &value, &units))\
			svg->viewBox.w = value;\
		else\
			LOG ("error parsing viewbox w: %s\n", strW);\
		if (try_parse_lenghtOrPercentage(strH, &value, &units))\
			svg->viewBox.h = value;\
		else\
			LOG ("error parsing viewbox h: %s\n", strH);\
		hasViewBox = true;\
	}\
}
#define PROCESS_SVG_WIDTH	_parse_lenghtOrPercentage(svg, &width);
#define PROCESS_SVG_HEIGHT	_parse_lenghtOrPercentage(svg, &height);
//=============================

//========G============
#define HEADING_G				vkvg_save (svg->ctx);
#define ELEMENT_PRE_PROCESS_G
#define ELEMENT_POST_PROCESS_G	vkvg_restore (svg->ctx);

//=============================

//========LINE============
#define HEADING_LINE \
	vkvg_save (svg->ctx);\
	svg_element_line* l = _new_line ();

#define ELEMENT_PRE_PROCESS_LINE	_process_element(svg, &attribs, l, false);
#define ELEMENT_POST_PROCESS_LINE	vkvg_restore (svg->ctx);

#define PROCESS_LINE_X1		_parse_lenghtOrPercentage(svg, &l->x1);
#define PROCESS_LINE_Y1		_parse_lenghtOrPercentage(svg, &l->y1);
#define PROCESS_LINE_X2		_parse_lenghtOrPercentage(svg, &l->x2);
#define PROCESS_LINE_Y2		_parse_lenghtOrPercentage(svg, &l->y2);
//=============================

//========RECT============
#define HEADING_RECT \
	vkvg_save (svg->ctx);\
	svg_element_rect* r = _new_rect();

#define ELEMENT_PRE_PROCESS_RECT	_process_element(svg, &attribs, r, false);
#define ELEMENT_POST_PROCESS_RECT	vkvg_restore (svg->ctx);

#define PROCESS_RECT_X		_parse_lenghtOrPercentage(svg, &r->x);
#define PROCESS_RECT_Y		_parse_lenghtOrPercentage(svg, &r->y);
#define PROCESS_RECT_WIDTH	_parse_lenghtOrPercentage(svg, &r->w);
#define PROCESS_RECT_HEIGHT	_parse_lenghtOrPercentage(svg, &r->h);
#define PROCESS_RECT_RX		_parse_lenghtOrPercentage(svg, &r->rx);
#define PROCESS_RECT_RY		_parse_lenghtOrPercentage(svg, &r->ry);
//========================

//========CIRCLE============
#define HEADING_CIRCLE \
	vkvg_save (svg->ctx);\
	svg_element_circle* c = _new_circle();

#define ELEMENT_PRE_PROCESS_CIRCLE	_process_element(svg, &attribs, c, false);
#define ELEMENT_POST_PROCESS_CIRCLE vkvg_restore (svg->ctx);

#define PROCESS_CIRCLE_CX	_parse_lenghtOrPercentage(svg, &c->cx);
#define PROCESS_CIRCLE_CY	_parse_lenghtOrPercentage(svg, &c->cy);
#define PROCESS_CIRCLE_R	_parse_lenghtOrPercentage(svg, &c->r);
//=============================

//========ELLIPSE============
#define HEADING_ELLIPSE \
	vkvg_save (svg->ctx);\
	svg_element_ellipse* e = _new_ellipse();

#define ELEMENT_PRE_PROCESS_ELLIPSE		_process_element(svg, &attribs, e, false);
#define ELEMENT_POST_PROCESS_ELLIPSE	vkvg_restore (svg->ctx);

#define PROCESS_ELLIPSE_CX	_parse_lenghtOrPercentage(svg, &e->cx);
#define PROCESS_ELLIPSE_CY	_parse_lenghtOrPercentage(svg, &e->cy);
#define PROCESS_ELLIPSE_RX	_parse_lenghtOrPercentage(svg, &e->rx);
#define PROCESS_ELLIPSE_RY	_parse_lenghtOrPercentage(svg, &e->ry);
//=============================


//========POLYLINE============
#define HEADING_POLYLINE vkvg_save (svg->ctx);
#define ELEMENT_PRE_PROCESS_POLYLINE \
	if (attribs.hasFill || attribs.hasStroke)\
		draw (svg, &attribs);
#define ELEMENT_POST_PROCESS_POLYLINE vkvg_restore (svg->ctx);

#define PROCESS_POLYLINE_POINTS _parse_point_list (svg);
//=============================

//========POLYGON============
#define HEADING_POLYGON vkvg_save (svg->ctx);
#define ELEMENT_PRE_PROCESS_POLYGON \
	if (attribs.hasFill || attribs.hasStroke) {\
		vkvg_close_path (svg->ctx);\
		draw (svg, &attribs);\
	}
#define ELEMENT_POST_PROCESS_POLYGON vkvg_restore (svg->ctx);
#define PROCESS_POLYGON_POINTS _parse_point_list (svg);
//=============================

//========PATH============
#define HEADING_PATH \
	vkvg_save (svg->ctx);\
	svg_element_path* p = _new_path ();

#define ELEMENT_PRE_PROCESS_PATH	_process_element(svg, &attribs, p, false);
#define ELEMENT_POST_PROCESS_PATH	vkvg_restore (svg->ctx);

#define PROCESS_PATH_D \
{\
	int len = strlen(svg->value);\
	if (len > 0) {\
		p->d = (char*)malloc(len*sizeof(char)+1);\
		strcpy (p->d, svg->value);\
	}\
}
//=============================

//========TEXT============
#define HEADING_TEXT \
	vkvg_save (svg->ctx);\
	svg_length_or_percentage x = {0}, y = {0}, dx = {0}, dy = {0};

#define ELEMENT_PRE_PROCESS_TEXT
#define ELEMENT_POST_PROCESS_TEXT \
	_draw_text (svg, f, &attribs, &x, &y, &dx, &dy);\
	vkvg_restore (svg->ctx);

#define PROCESS_TEXT_X	_parse_lenghtOrPercentage(svg, &x);
#define PROCESS_TEXT_Y	_parse_lenghtOrPercentage(svg, &y);
#define PROCESS_TEXT_DX	_parse_lenghtOrPercentage(svg, &dx);
#define PROCESS_TEXT_DY	_parse_lenghtOrPercentage(svg, &dy);


//=============================


//========LINEARGRADIENT============
#define HEADING_LINEARGRADIENT \
	svg_element_linear_gradient* rg = _new_linear_gradient();\
	bool hasTransform = false;\
	vkvg_matrix_t transform = VKVG_IDENTITY_MATRIX;

#define ELEMENT_PRE_PROCESS_LINEARGRADIENT \
	rg->pattern = vkvg_pattern_create_linear (rg->x1.number, rg->y1.number, rg->x2.number, rg->y2.number);\
	rg->id.hash = svg->currentIdHash;\
	rg->id.xlinkHref = svg->currentXlinkHref;\
	LOG ("store pattern id:%u href:%u\n", rg->id.hash, rg->id.xlinkHref);\
	if (hasTransform)\
		vkvg_pattern_set_matrix(rg->pattern, &transform);\
	array_add (svg->idList, rg);\
	parentData = (void*)rg->pattern;

#define PROCESS_LINEARGRADIENT_X1 _parse_lenghtOrPercentage(svg, &rg->x1);
#define PROCESS_LINEARGRADIENT_Y1 _parse_lenghtOrPercentage(svg, &rg->y1);
#define PROCESS_LINEARGRADIENT_X2 _parse_lenghtOrPercentage(svg, &rg->x2);
#define PROCESS_LINEARGRADIENT_Y2 _parse_lenghtOrPercentage(svg, &rg->y2);
#define PROCESS_LINEARGRADIENT_GRADIENTUNITS_USERSPACEONUSE		rg->gradientUnits = svg_gradient_unit_userSpaceOnUse;
#define PROCESS_LINEARGRADIENT_GRADIENTUNITS_OBJECTBOUNDINGBOX	rg->gradientUnits = svg_gradient_unit_objectBoundingBox;
#define PROCESS_LINEARGRADIENT_GRADIENTTRANSFORM	hasTransform = _try_parse_transform(svg, &transform);
//=============================


//========RADIALGRADIENT============
#define HEADING_RADIALGRADIENT \
	svg_element_radial_gradient* rg = _new_radial_gradient();\
	bool hasTransform = false;\
	vkvg_matrix_t transform = VKVG_IDENTITY_MATRIX;

#define ELEMENT_PRE_PROCESS_RADIALGRADIENT \
	rg->pattern = vkvg_pattern_create_radial (rg->fx.number, rg->fy.number, 0, rg->cx.number, rg->cy.number, rg->r.number);\
	rg->id.hash = svg->currentIdHash;\
	rg->id.xlinkHref = svg->currentXlinkHref;\
	LOG ("store pattern id:%u href:%u\n", rg->id.hash, rg->id.xlinkHref);\
	if (hasTransform)\
		vkvg_pattern_set_matrix(rg->pattern, &transform);\
	array_add (svg->idList, rg);\
	parentData = (void*)rg->pattern;

#define PROCESS_RADIALGRADIENT_CX _parse_lenghtOrPercentage(svg, &rg->cx);
#define PROCESS_RADIALGRADIENT_CY _parse_lenghtOrPercentage(svg, &rg->cy);
#define PROCESS_RADIALGRADIENT_R  _parse_lenghtOrPercentage(svg, &rg->r);
#define PROCESS_RADIALGRADIENT_FX _parse_lenghtOrPercentage(svg, &rg->fx);
#define PROCESS_RADIALGRADIENT_FY _parse_lenghtOrPercentage(svg, &rg->fy);
#define PROCESS_RADIALGRADIENT_GRADIENTUNITS_USERSPACEONUSE		rg->gradientUnits = svg_gradient_unit_userSpaceOnUse;
#define PROCESS_RADIALGRADIENT_GRADIENTUNITS_OBJECTBOUNDINGBOX	rg->gradientUnits = svg_gradient_unit_objectBoundingBox;
#define PROCESS_RADIALGRADIENT_GRADIENTTRANSFORM PROCESS_LINEARGRADIENT_GRADIENTTRANSFORM


//=============================

//========STOP============
#define HEADING_STOP \
	VkvgPattern pat = (VkvgPattern)parentData;\
	float offset = 0;\
	svg_unit offset_unit = svg_unit_px;\

//todo multiple compress/decompress of colors
#define ELEMENT_PRE_PROCESS_STOP \
	float  a = (float)((stop_color&0xff000000)>>24) / 255.0f;\
	float  b = (float)((stop_color&0x00ff0000)>>16) / 255.0f;\
	float  g = (float)((stop_color&0x0000ff00)>> 8) / 255.0f;\
	float  r = (float)( stop_color&0x000000ff) / 255.0f;\
	vkvg_pattern_add_color_stop(pat, offset, r, g, b, a * grad_opacity);

#define PROCESS_STOP_OFFSET \
{\
	if (!try_parse_lenghtOrPercentage(svg->value, &offset, &offset_unit))\
		LOG ("error parsing gradient offset: %s\n", svg->value);\
	switch (offset_unit) {\
	case svg_unit_percentage:\
		offset /= 100.0f;\
		break;\
	}\
}

#define HEADING_SVG_GRADIENT_ATTRIB \
	uint32_t stop_color = 0;\
	float grad_opacity = 1.0f;

#define PROCESS_SVG_GRADIENT_ATTRIB_STOP_COLOR {\
	svg_paint_type enabled;\
	try_parse_color(svg->value, &enabled, &stop_color);\
}

#define PROCESS_SVG_GRADIENT_ATTRIB_STOP_OPACITY grad_opacity = _parse_opacity (svg);


//=============================

//========DEFS============
#define HEADING_DEFS
#define ELEMENT_PRE_PROCESS_DEFS	svg->skipDraw = true;
#define ELEMENT_POST_PROCESS_DEFS	svg->skipDraw = false;
//=============================


//========USE============
#define HEADING_USE
#define ELEMENT_PRE_PROCESS_USE		_process_use(svg, &attribs);
#define ELEMENT_POST_PROCESS_USE

/*#define PROCESS_USE_X
#define PROCESS_USE_Y
#define PROCESS_USE_WIDTH
#define PROCESS_USE_HEIGHT*/

//=============================

#define PARSER_GEN_IMPLEMENTATION
#include "parser_gen.h"

int skip_children (svg_context* svg, FILE* f, svg_attributes attribs, void *parentData) {
	int res = 0;
	while (!feof (f)) {
		read_element_start
		skip_element
	}
	return res;
}

int skip_attributes_and_children (svg_context* svg, FILE* f, svg_attributes attribs) {
	int res = get_attribute;
	while (res > 0) {
		if (res == 1 && (svg->att[0] == '/' || svg->att[0] == '?'))
			break;//self closing tag
		res = get_attribute;
	}
	read_tag_end
	if (res>0)
		res = skip_children (svg, f, attribs, NULL);
	return res;
}

int read_tag (svg_context* svg, FILE* f, svg_attributes attribs) {
	int res = 0;
	while (!feof (f)) {

		read_element_start

		if (!strcasecmp(svg->elt,"svg"))
			res = read_svg_attributes	(svg, f, attribs, NULL);
		else
			skip_element
	}
	return res;
}

VkvgSurface vkvg_create_surface_from_svg (VkvgDevice dev, const char* filename, uint32_t width, uint32_t height) {
	FILE* f = fopen(filename, "r");
	if (f == NULL){
		perror ("vkvg_svg: file not found");
		return NULL;
	}
	LOG ("loading %s\n", filename);

	svg_context svg = {0};
	svg.dev		= dev;
	svg.width	= width;
	svg.height	= height;
	svg.fit		= true;
	svg.idList	= array_create();

	svg_attributes attribs = {
		svg_paint_type_solid, svg_paint_type_none, svg_paint_type_solid, 0xff000000, 0xff000000, 0xff000000,
		1.0f,1.0f,1.0f,//opacities
		svg_text_anchor_start,
	};

	read_tag (&svg, f, attribs);

	fclose(f);

	for (uint32_t i=0; i<svg.idList->count; i++) {
		switch (_get_element_type (svg.idList->elements[i])) {
		case svg_element_type_linear_gradient:
			vkvg_pattern_destroy((VkvgPattern) ((svg_element_linear_gradient*)svg.idList->elements[i])->pattern);
			break;
		case svg_element_type_radial_gradient:
			vkvg_pattern_destroy((VkvgPattern) ((svg_element_radial_gradient*)svg.idList->elements[i])->pattern);
			break;
		case svg_element_type_path:
			{
				CASTELT(p,path,svg.idList->elements[i]);
				if (p->d)
					free (p->d);
			}
			break;
		}
		free (svg.idList->elements[i]);
	}
	array_destroy (svg.idList);

	return svg.surf;
}
