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
	} else if (!strcmp (colorString, "none"))
		*isEnabled = false;
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
	else if (!strcmp(att, "stroke"))
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
					vkvg_matrix_multiply(&newMat, &current, &m);
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

int read_path_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			float x, y, c1x, c1y, c2x, c2y, cpX, cpY;
			bool hasCP = false;
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
						if (parse_floats(tmp, 4, &c2x, &c2y, &x, &y))
							vkvg_quadratic_to (ctx, c2x, c2y, x, y);
						else
							parseError = true;
						break;
					case 'q':
						if (parse_floats(tmp, 4, &c2x, &c2y, &x, &y))
							vkvg_rel_quadratic_to (ctx, c2x, c2y, x, y);
						else
							parseError = true;
						break;
					case 'C':
						if (repeat)
							result = parse_floats(tmp, 5, &c1y, &c2x, &c2y, &x, &y);
						else
							result = parse_floats(tmp, 6, &c1x, &c1y, &c2x, &c2y, &x, &y);
						if (result) {
							vkvg_curve_to (ctx, c1x, c1y, c2x, c2y, x, y);
							hasCP = true;
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
							hasCP = true;
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
							if (hasCP) {
								vkvg_curve_to (ctx, 2.0 * cpX - c2x, 2.0 * cpY - c2y, c1x, c1y, x, y);
							} else
								vkvg_curve_to (ctx, cpX, cpY, c1x, c1y, x, y);

							c2x = c1x;
							c2y = c1y;
							hasCP = true;
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
							if (hasCP) {
								vkvg_get_current_point(ctx, &cpX, &cpY);
								vkvg_rel_curve_to (ctx, cpX - c2x, cpY - c2y, c1x, c1y, x, y);
							} else
								vkvg_rel_curve_to (ctx, 0, 0, c1x, c1y, x, y);

							c2x = cpX + c1x;
							c2y = cpY + c1y;
							hasCP = true;
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
					hasCP = false;
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
