#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <vkvg.h>
#include "vkengine.h"

static int res;
VkvgDevice dev;
static VkvgSurface svgSurf = NULL;
static VkvgContext ctx;

#define get_attribute fscanf(f, " %[^=>]=%*[\"']%[^\"']%*[\"']", att, value)

static int (*read_attributes_ptr)(FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill);

/*static uint32_t strokes[32];
static uint32_t fills[32];
static int strokesPtr = -1 fillsPtr = -1;

void push_stroke_color (uint32_t stroke) {
	strokes[++strokesPtr] = stroke;
}*/
static char att[1024];
static char value[5096];

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
		char hexColorString[9];
		if (strlen(color) == 3)
			sprintf(hexColorString, "0x%c%c%c%c%c%c", color[0],color[0],color[1],color[1],color[2],color[2]);
		else
			sprintf(hexColorString, "0x%s", color);
		sscanf(hexColorString, "%x", &colorValue);
		*isEnabled = true;
		colorValue = (colorValue << 8) | 0xFF;
	} else if (!strcmp (colorString, "none"))
		*isEnabled = false;
	return colorValue;
}
int read_common_attributes (bool *hasStroke, bool *hasFill, uint32_t *stroke, uint32_t *fill) {
	if (!strcmp(att, "fill"))
		*fill = parse_color (value, hasFill);
	else if (!strcmp(att, "stroke"))
		*stroke = parse_color (value, hasStroke);
	else if (!strcmp (att, "stroke-width"))
		vkvg_set_line_width(ctx, parse_lenghtOrPercentage (value));
	else
		return 0;
	return 1;
}
int read_svg_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	bool hasViewBox = false;
	int x, y, w, h, width = 0, height = 0;
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			if (!strcmp(att, "viewBox")) {
				res = sscanf(value, " %d%*1[ ,]%d%*1[ ,]%d%*1[ ,]%d", &x, &y, &w, &h);
				if (res!=4)
					return -1;
				hasViewBox = true;
			} else if (!strcmp(att, "width"))
				width = parse_lenghtOrPercentage (value);
			else if (!strcmp(att, "height"))
				height = parse_lenghtOrPercentage (value);
			else
				printf("Unprocessed attribute: %s\n", att);
		}
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

	svgSurf = vkvg_surface_create(dev, surfW, surfH);
	ctx = vkvg_create(svgSurf);
	vkvg_scale(ctx, xScale, yScale);
	vkvg_clear(ctx);

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
				printf("Unprocessed attribute: %s\n", att);
		}
		res = get_attribute;
	}
	if (w && h && (hasFill || hasStroke)) {
		vkvg_rectangle(ctx, x, y, w, h);
		if (hasFill) {
			vkvg_set_source_color(ctx, fill);
			if (hasStroke) {
				vkvg_fill_preserve(ctx);
				vkvg_set_source_color(ctx, stroke);
				vkvg_stroke(ctx);
			} else
				vkvg_fill (ctx);
		} if (hasStroke) {
			vkvg_set_source_color(ctx, stroke);
			vkvg_stroke(ctx);
		}
	}
	return res;
}

int read_g_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	vkvg_save (ctx);
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			printf("Unprocessed attribute: %s\n", att);
		}
		res = get_attribute;
	}
	return res;
}

int read_attributes (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	res = get_attribute;
	while (res == 2) {
		if (!read_common_attributes(&hasStroke, &hasFill, &stroke, &fill)) {
			printf("Unprocessed attribute: %s\n", att);
		}
		res = get_attribute;
	}
	return res;
}
int read_tag (FILE* f, bool hasStroke, bool hasFill, uint32_t stroke, uint32_t fill) {
	char elt[128];
	res = fscanf(f, " <%[^> ]", elt);
	if (res < 0)
		return 0;
	if (!res) {
		printf("element name parsing error\n");
		return -1;
	}
	if (elt[0] == '/') {
		if (getc(f) != '>') {
			printf("parsing error, expecting '>'\n");
			return -1;
		}
		if (!strcmp(elt,"/svg"))
			vkvg_destroy(ctx);
		else if (!strcmp(elt,"/g")) {
			vkvg_restore(ctx);
		}
		read_attributes_ptr = NULL;
		return 0;
	}
	if (!strcmp(elt,"svg"))
		res = read_svg_attributes	(f, hasStroke, hasFill, stroke, fill);
	else if (!strcmp(elt,"g"))
		res = read_g_attributes		(f, hasStroke, hasFill, stroke, fill);
	else if (!strcmp(elt,"rect"))
		res = read_rect_attributes	(f, hasStroke, hasFill, stroke, fill);
	else
		res = read_attributes		(f, hasStroke, hasFill, stroke, fill);

	if (res < 0) {
		printf("error parsing: %s\n", att);
		return -1;
	}
	if (res == 1) {
		if (getc(f) != '>') {
			printf("parsing error, expecting '>'\n");
			return -1;
		}
		return 0;
	}
	if (getc(f) != '>') {
		printf("parsing error, expecting '>'\n");
		return -1;
	}
	return read_tag(f, hasStroke, hasFill, stroke, fill);
}


static VkSampleCountFlags samples = VK_SAMPLE_COUNT_8_BIT;
static uint32_t width=800, height=600;
static bool paused = false;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action != GLFW_PRESS)
		return;
	switch (key) {
	case GLFW_KEY_SPACE:
		 paused = !paused;
		break;
	case GLFW_KEY_ESCAPE :
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		break;
	}
}



int main (int argc, char *argv[]){
	char* filename;
	if (argc > 1)
		filename = argv[1];
	else
		filename = "images/tiger.svg";

	VkEngine e = vkengine_create (VK_PRESENT_MODE_MAILBOX_KHR, width, height);
	vkengine_set_key_callback (e, key_callback);
	dev = vkvg_device_create_multisample(vkh_app_get_inst(e->app),
			 vkengine_get_physical_device(e), vkengine_get_device(e), vkengine_get_queue_fam_idx(e), 0, samples, false);
	VkvgSurface surf = vkvg_surface_create(dev, width, height);

	vkh_presenter_build_blit_cmd (e->renderer, vkvg_surface_get_vk_image(surf), width, height);

	FILE* f = fopen(filename, "r");
	if (f == NULL){
		printf("error: %d\n", errno);
		return -1;
	}
	//char elt[128];

	while (!feof(f)) {
		if (read_tag (f, false, false, 0, 0) < 0)
			break;
	}
	fclose(f);

	while (!vkengine_should_close (e)) {
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
		vkvg_set_source_surface(ctx, svgSurf, 0, 0);
		vkvg_paint(ctx);
		vkvg_destroy(ctx);
	}
	vkvg_surface_destroy(surf);
	vkvg_device_destroy(dev);
	vkengine_destroy(e);
}
/*int main(){
	const char* filename = "/mnt/data/images/svg/tiger.svg";
	FILE* f = fopen(filename, "r");
	if (f == NULL){
		printf("error: %d\n", errno);
		return -1;
	}
	char elt[128];
	char att[1024];
	char value[5096];

	while (!feof(f)) {
		//fscanf(f, "[ |\n|\r|\t]");//skip whitespaces
		int res = fscanf(f, " <%[^> ]", elt);
		if (res < 0)
			break;
		if (!res) {
			printf("element name parsing error\n");
			return -1;
		}
		if (elt[0] == '/') {
			printf("\n<%s ", elt);
			res = fscanf(f, "  %c", elt);
			if (!res || elt[0] != '>') {
				printf("parsing error, expecting '>'\n");
				return -1;
			}
			printf(">\n");
			continue;;
		}
		printf("<%s ", elt);
		fflush(stdout);

		res = fscanf(f, " %[^=>]=%*[\"']%[^\"']%*[\"']", att, value);
		while (res == 2) {
			printf("%s='%s' ", att, value);
			fflush(stdout);
			res = fscanf(f, " %[^=>]=%*[\"']%[^\"']%*[\"']", att, value);
		}
		if (res == 0)
			printf(">\n");
		else if (res == 1)
			printf("%s>\n", att);
		if (getc(f) != '>') {
			printf("parsing error, expecting '>'\n");
			return -1;
		}
		fflush(stdout);
	}



	fclose(f);
}*/
