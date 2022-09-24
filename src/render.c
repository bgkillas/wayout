#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>
#include<math.h>

#include<cairo/cairo.h>
#include<pango/pangocairo.h>
#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wayout.h"
#include"surface.h"
#include"output.h"
#include"misc.h"
#include"colour.h"
#include"render.h"

#define PI 3.141592653589793238462643383279502884

static void rounded_rectangle (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
		double tl_r, double tr_r, double bl_r, double br_r)
{
	double degrees = PI / 180.0;
	cairo_new_sub_path(cairo);
	cairo_arc(cairo, x + w - tr_r, y     + tr_r, tr_r, -90 * degrees,   0 * degrees);
	cairo_arc(cairo, x + w - br_r, y + h - br_r, br_r,   0 * degrees,  90 * degrees);
	cairo_arc(cairo, x +     bl_r, y + h - bl_r, bl_r,  90 * degrees, 180 * degrees);
	cairo_arc(cairo, x +     tl_r, y     + tl_r, tl_r, 180 * degrees, 270 * degrees);
	cairo_close_path(cairo);
}

static void draw_background (cairo_t *cairo, struct Draw_dimensions *dimensions,
		int32_t scale, struct App *app)
{
	if ( colour_is_transparent(&app->background_colour)
			&& colour_is_transparent(&app->border_colour) )
		return;

	int32_t w                   = scale * dimensions->w;
	int32_t h                   = scale * dimensions->h;
	int32_t check_size          = (dimensions->w > dimensions->h) ? scale * dimensions->w : scale * dimensions->h;
	int32_t radius_top_left     = scale * app->radius_top_left;
	int32_t radius_top_right    = scale * app->radius_top_right;
	int32_t radius_bottom_left  = scale * app->radius_bottom_left;
	int32_t radius_bottom_right = scale * app->radius_bottom_right;
	int32_t border_left = scale * app->border_left;
	int32_t border_top = scale * app->border_top;
	int32_t border_right = scale * app->border_right;
	int32_t border_bottom = scale * app->border_bottom;

	/* Avoid too radii so big that they cause unexpected drawing behaviour. */
	if ( radius_top_left > check_size / 2 )
		radius_top_left = check_size / 2;
	if ( radius_top_right > check_size / 2 )
		radius_top_right = check_size / 2;
	if ( radius_bottom_left > check_size / 2 )
		radius_bottom_left = check_size / 2;
	if ( radius_bottom_right > check_size / 2 )
		radius_bottom_right = check_size / 2;

	printlog(app, 3, "[render] Render dimensions (scaled): w=%d h=%d\n", w, h);

	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);

	if ( radius_top_left == 0 && radius_top_right == 0
			&& radius_bottom_left == 0 && radius_bottom_right == 0 )
	{
		if (app->border_left || app->border_right || app->border_top || app->border_bottom) {
			cairo_rectangle(cairo, 0, 0, w, h);
			colour_set_cairo_source(cairo, &app->border_colour);
			cairo_fill(cairo);
		}

		cairo_rectangle(cairo, border_left, border_top, w - border_left - border_right, h - border_top - border_bottom);
		colour_set_cairo_source(cairo, &app->background_colour);
		cairo_fill(cairo);
	}
	else
	{
		if (app->border_left || app->border_right || app->border_top || app->border_bottom) {
			rounded_rectangle(cairo, 0, 0, w, h,
				radius_top_left, radius_top_right,
				radius_bottom_left, radius_bottom_right);
			colour_set_cairo_source(cairo, &app->border_colour);
			cairo_fill(cairo);
		}

		rounded_rectangle(cairo, border_left, border_top, w - border_left - border_right, h - border_top - border_bottom,
				radius_top_left, radius_top_right,
				radius_bottom_left, radius_bottom_right);
		colour_set_cairo_source(cairo, &app->background_colour);
		cairo_fill(cairo);
	}

	cairo_restore(cairo);
}

static void draw_main (cairo_t *cairo, PangoLayout *layout, PangoFontDescription * font_description, struct Draw_dimensions *dimensions,
		int32_t scale, struct App *app)
{
	cairo_save(cairo);

	cairo_set_source_rgba (
		cairo,
		app->text_colour.r,
		app->text_colour.g,
		app->text_colour.b,
		app->text_colour.a
	);
	int32_t w = app->dimensions.w * scale;
	int32_t h = app->dimensions.h * scale;
	if (app->text) {
		printlog(app, 2, "Outputting text: %s\n", app->text);
		cairo_move_to(cairo, w / 2.0, h / 2.0);
		pango_layout_set_font_description(layout, font_description);
		if (app->center) pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
time_t current_time;
struct tm * time_info;
char timeString[9];


printf("%s\n","t");
time(&current_time);
time_info = localtime(&current_time);
strftime(timeString, 9, "%H:%M:%S", time_info);
pango_layout_set_markup(layout, timeString, -1);

}
		int width, height;
		pango_layout_get_size(layout, &width, &height);

		if (app->center) {
			cairo_rel_move_to(cairo, - w / 2.0, - h / 2.0);
		} else {
			cairo_rel_move_to(cairo, - ((double)width / PANGO_SCALE) / 2, - ((double)height / PANGO_SCALE) / 2);
		}
	}
	pango_cairo_show_layout(cairo, layout);
	cairo_restore(cairo);
}

static void clear_buffer (cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

void render_background_frame (struct Draw_surface *surface)
{
	struct Draw_output *output = surface->output;
	struct App        *app  = output->app;
	uint32_t               scale  = output->scale;

	printlog(app, 2, "[render] Render background frame: global_name=%d\n",
			output->global_name);

	if (! next_buffer(&surface->current_background_buffer, app->shm,
				surface->background_buffers,
				surface->dimensions.w * scale,
				surface->dimensions.h * scale))
		return;
	surface->current_background_buffer->busy = true;

	cairo_t *cairo = surface->current_background_buffer->cairo;

	PangoLayout * layout = pango_cairo_create_layout(cairo);
	if (app->wordwrap) {
		pango_layout_set_width (layout, app->dimensions.w * scale * PANGO_SCALE);
		pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	}
	clear_buffer(cairo);

	draw_background(cairo, &surface->dimensions, scale, app);
	draw_main(cairo, layout, surface->font_description, &surface->dimensions,  scale, app);

	wl_surface_set_buffer_scale(surface->background_surface, scale);
	wl_surface_damage_buffer(surface->background_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_attach(surface->background_surface, surface->current_background_buffer->buffer, 0, 0);
}
