#ifndef WLCLOCK_OUTPUT_H
#define WLCLOCK_OUTPUT_H

#include<wayland-server.h>

struct App;
struct Draw_surface;

struct Draw_output
{
	struct wl_list  link;
	struct App *app;

	struct wl_output      *wl_output;
	struct zxdg_output_v1 *xdg_output;

	char     *name;
	uint32_t  global_name;
	uint32_t  scale;

	bool configured;

	struct Draw_surface *surface;
};

bool create_output (struct App *app, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version);
bool configure_output (struct Draw_output *output);
struct Draw_output *get_output_from_global_name (struct App *app, uint32_t name);
void destroy_output (struct Draw_output *output);
void destroy_all_outputs (struct App *app);

#endif
