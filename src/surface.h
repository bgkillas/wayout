#ifndef WLCLOCK_SURFACE_H
#define WLCLOCK_SURFACE_H

#include<wayland-server.h>

#include"buffer.h"
#include"wayout.h"
#include<pango/pangocairo.h>

#include<stdint.h>
#include<stdbool.h>

struct App;
struct Draw_output;

struct Draw_surface
{
	struct Draw_output        *output;
	struct wl_surface            *background_surface;
	struct wl_surface            *hands_surface;
	struct wl_subsurface         *subsurface;
	struct zwlr_layer_surface_v1 *layer_surface;

	struct Draw_dimensions dimensions;
	struct Draw_buffer  background_buffers[2];
	struct Draw_buffer *current_background_buffer;
	PangoFontDescription *font_description;
	bool configured;
};

bool create_surface (struct Draw_output *output);
void destroy_surface (struct Draw_surface *surface);
void update (struct App *app);

#endif
