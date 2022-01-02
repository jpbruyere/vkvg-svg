#include "vkvg_extensions.h"
#include "vkvg_context_internal.h"
#include "vectors.h"


static vec2 _p0, _p1, _v1, _v2, _c;
void _drawhilight (VkvgContext ctx, vec2 p, float r, float g, float b) {
	vkvg_set_source_rgb(ctx,r,g,b);
	vkvg_arc(ctx,p.x,p.y,5,0,M_PIF*2);
	vkvg_fill(ctx);
}
void _highlight (VkvgContext ctx) {
	_drawhilight (ctx, _p0, 1,0,0);
	_drawhilight (ctx, _p1, 0,1,0);
	_drawhilight (ctx, _c, 0,0,1);

	vkvg_move_to(ctx, _c.x,_c.y);
	vkvg_line_to(ctx, _v1.x,_v1.y);
	vkvg_set_source_rgb(ctx, 1,0,0);
	vkvg_stroke(ctx);
	/*vkvg_move_to(ctx, _c.x,_c.y);
	vkvg_line_to(ctx, _v2.x,_v2.y);
	vkvg_set_source_rgb(ctx, 0,1,0);
	vkvg_stroke(ctx);*/
}
void vkvg_elliptical_arc (VkvgContext ctx, float x2, float y2, bool largeArc, bool counterClockWise, float rx, float ry, float phi) {
	float x1, y1;
	vkvg_get_current_point(ctx, &x1, &y1);
	vkvg_elliptic_arc(ctx, x1, y1, x2, y2, largeArc, counterClockWise, rx, ry, phi);
}
void vkvg_elliptic_arc (VkvgContext ctx, float x1, float y1, float x2, float y2, bool largeArc, bool counterClockWise, float rx, float ry, float phi) {
	if (ctx->status)
		return;

	mat2 m = {
		{ cosf (phi), sinf (phi)},
		{-sinf (phi), cosf (phi)}
	};
	vec2 p = {(x1 - x2)/2, (y1 - y2)/2};
	vec2 p1 = mat2_mult_vec2 (m, p);

	//radii corrections
	double lambda = powf (p1.x, 2) / powf (rx, 2) + powf (p1.y, 2) / powf (ry, 2);
	if (lambda > 1) {
		lambda = sqrtf (lambda);
		rx *= lambda;
		ry *= lambda;
	}

	p = (vec2){rx * p1.y / ry, -ry * p1.x / rx};

	vec2 cp = vec2_mult_s (p, sqrtf (fabsf (
		(powf (rx,2) * powf (ry,2) - powf (rx,2) * powf (p1.y, 2) - powf (ry,2) * powf (p1.x, 2)) /
		(powf (rx,2) * powf (p1.y, 2) + powf (ry,2) * powf (p1.x, 2))
	)));

	if (largeArc == counterClockWise)
		vec2_inv(&cp);

	m = (mat2) {
		{cosf (phi),-sinf (phi)},
		{sinf (phi), cosf (phi)}
	};
	p = (vec2){(x1 + x2)/2, (y1 + y2)/2};
	vec2 c = vec2_add (mat2_mult_vec2(m, cp) , p);

	vec2 u = vec2_unit_x;
	vec2 v = {(p1.x-cp.x)/rx, (p1.y-cp.y)/ry};
	double sa = acosf (vec2_dot (u, v) / (fabsf(vec2_length(v)) * fabsf(vec2_length(u))));
	if (isnanf(sa))
		sa=M_PIF;
	if (u.x*v.y-u.y*v.x < 0)
		sa = -sa;

	u = v;
	v = (vec2) {(-p1.x-cp.x)/rx, (-p1.y-cp.y)/ry};
	double delta_theta = acosf (vec2_dot (u, v) / (fabsf(vec2_length (v)) * fabsf(vec2_length (u))));
	if (isnanf(delta_theta))
		delta_theta=M_PIF;
	if (u.x*v.y-u.y*v.x < 0)
		delta_theta = -delta_theta;

	//_v1 = vec2_add(vec2_mult(vec2_sub(v,c),1),c);

	if (counterClockWise) {
		if (delta_theta < 0)
			delta_theta += M_PIF * 2.0;
	} else if (delta_theta > 0)
		delta_theta -= M_PIF * 2.0;

	//_v2 = vec2_add(vec2_mult(vec2_sub(v,c),1),c);

	_c = c;

	m = (mat2) {
		{cosf (phi),-sinf (phi)},
		{sinf (phi), cosf (phi)}
	};

	double theta = sa;
	double ea = sa + delta_theta;

	float step = _get_arc_step(ctx, fminf (rx, ry))*0.1f;

	//_finish_path (ctx);

	p = (vec2) {
		rx * cosf (sa),
		ry * sinf (sa)
	};
	//_p0 = vec2_add (mat2_mult_vec2 (m, p), c);
	p = (vec2) {
		rx * cosf (ea),
		ry * sinf (ea)
	};
	//_p1 = vec2_add (mat2_mult_vec2 (m, p), c);

	_set_curve_start (ctx);

	//_add_point (ctx, c.x, c.y);

	if (sa < ea) {
		while (theta < ea) {
			p = (vec2) {
				rx * cosf (theta),
				ry * sinf (theta)
			};
			vec2 xy = vec2_add (mat2_mult_vec2 (m, p), c);
			_add_point (ctx, xy.x, xy.y);
			theta += step;
		}
	} else {
		while (theta > ea) {
			p = (vec2) {
				rx * cosf (theta),
				ry * sinf (theta)
			};
			vec2 xy = vec2_add (mat2_mult_vec2 (m, p), c);
			_add_point (ctx, xy.x, xy.y);
			theta -= step;
		}
	}
	p = (vec2) {
		rx * cosf (ea),
		ry * sinf (ea)
	};
	vec2 xy = vec2_add (mat2_mult_vec2 (m, p), c);
	_add_point (ctx, xy.x, xy.y);
	_set_curve_end(ctx);
}
void vkvg_ellipse (VkvgContext ctx, float radiusX, float radiusY, float x, float y, float rotationAngle) {
	float width_two_thirds = radiusX * 4 / 3;

	float dx1 = sinf(rotationAngle) * radiusY;
	float dy1 = cosf(rotationAngle) * radiusY;
	float dx2 = cosf(rotationAngle) * width_two_thirds;
	float dy2 = sinf(rotationAngle) * width_two_thirds;

	float topCenterX = x - dx1;
	float topCenterY = y + dy1;
	float topRightX = topCenterX + dx2;
	float topRightY = topCenterY + dy2;
	float topLeftX = topCenterX - dx2;
	float topLeftY = topCenterY - dy2;

	float bottomCenterX = x + dx1;
	float bottomCenterY = y - dy1;
	float bottomRightX = bottomCenterX + dx2;
	float bottomRightY = bottomCenterY + dy2;
	float bottomLeftX = bottomCenterX - dx2;
	float bottomLeftY = bottomCenterY - dy2;

	vkvg_move_to (ctx, bottomCenterX, bottomCenterY);
	vkvg_curve_to (ctx, bottomRightX, bottomRightY, topRightX, topRightY, topCenterX, topCenterY);
	vkvg_curve_to (ctx, topLeftX, topLeftY, bottomLeftX, bottomLeftY, bottomCenterX, bottomCenterY);
	vkvg_close_path (ctx);
}
void vkvg_rounded_rectangle2 (VkvgContext ctx, float x, float y, float w, float h, float rx, float ry){
	vkvg_move_to (ctx, x+rx, y);
	vkvg_line_to (ctx, x+w-rx, y);
	vkvg_elliptical_arc(ctx, x+w, y+ry, false, true, rx, ry, 0);

	vkvg_line_to (ctx, x+w, y+h-ry);
	vkvg_elliptical_arc(ctx, x+w-rx, y+h, false, true, rx, ry, 0);

	vkvg_line_to (ctx, x+rx, y+h);
	vkvg_elliptical_arc(ctx, x, y+h-ry , false, true, rx, ry, 0);

	vkvg_line_to (ctx, x, y+ry);
	vkvg_elliptical_arc(ctx, x+rx, y , false, true, rx, ry, 0);

	vkvg_close_path(ctx);
}
