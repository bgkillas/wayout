#ifndef WLCLOCK_WLCLOCK_H
#define WLCLOCK_WLCLOCK_H

#include<stdbool.h>
#include<stdint.h>
#include<time.h>
#include<wayland-server.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"

#include"colour.h"

struct Draw_dimensions
{
	/* Width and height of entire surface (including borders). */
	int32_t w, h;
};

struct App
{
	struct wl_display             *display;
	struct wl_registry            *registry;
	struct wl_compositor          *compositor;
	struct wl_subcompositor       *subcompositor;
	struct wl_shm                 *shm;
	struct zwlr_layer_shell_v1    *layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;

	struct wl_list outputs;
	char *output;

	bool loop;
	int  verbosity;
	int  ret;

	enum zwlr_layer_shell_v1_layer layer;
	struct Draw_dimensions dimensions;
	char *namespace;
	int32_t exclusive_zone;
	int32_t border_top, border_right, border_bottom, border_left;
	int32_t margin_top, margin_right, margin_bottom, margin_left;
	int32_t radius_top_left, radius_top_right, radius_bottom_left, radius_bottom_right;
	int32_t anchor;
	bool input, snap;

	bool feed;
	int32_t interval;
	char *delimiter;

	struct Draw_colour background_colour;
	struct Draw_colour border_colour;
	struct Draw_colour text_colour;

	char *font_pattern;
	char *text;

	bool require_update;
	bool ready;
	bool wordwrap;
	bool center;
};

#endif
