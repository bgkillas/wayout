#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>
#include<time.h>

#include<cairo/cairo.h>
#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"wayout.h"
#include"output.h"
#include"misc.h"
#include"surface.h"
#include"buffer.h"
#include"render.h"

static void layer_surface_handle_configure (void *data,
		struct zwlr_layer_surface_v1 *layer_surface, uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Draw_surface *surface = (struct Draw_surface *)data;
	struct App         *app   = surface->output->app;
	printlog(app, 1, "[surface] Layer surface configure request: global_name=%d w=%d h=%d serial=%d\n",
			surface->output->global_name, w, h, serial);

	zwlr_layer_surface_v1_ack_configure(layer_surface, serial);

	bool dimensions_changed = false;
	if ( w != (uint32_t)surface->dimensions.w )
	{
		/* A size of 0 means we are free to use whatever size we want.
		 * So let's just use the one the user configured.
		 */
		surface->dimensions.w = w == 0 ? app->dimensions.w : (int32_t)w;
		dimensions_changed = true;
	}
	if ( h != (uint32_t)surface->dimensions.h )
	{
		surface->dimensions.w = w == 0 ? app->dimensions.h : (int32_t)h;
		dimensions_changed = true;
	}

	/* We can only attach buffers to a surface after it has received a
	 * configure event. A new layer surface will get a configure event
	 * immediately. As a clean way to do the first render, we therefore
	 * always render on the first configure event.
	 */
	if ( dimensions_changed || !surface->configured )
	{
		surface->configured = true;
		app->ready = true;

		render_background_frame(surface);
		wl_surface_commit(surface->background_surface);
	}
}

static void layer_surface_handle_closed (void *data, struct zwlr_layer_surface_v1 *layer_surface)
{
	/* This event is received when the compositor closed our surface, for
	 * example because the output is was on disappeared. We now need to
	 * cleanup the surfaces remains. Should the output re-appear, then it
	 * will receive a new one.
	 */
	struct Draw_surface *surface = (struct Draw_surface *)data;
	printlog(surface->output->app, 1,
			"[surface] Layer surface has been closed: global_name=%d\n",
			surface->output->global_name);
	destroy_surface(surface);
}

const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = layer_surface_handle_closed
};

static int32_t get_exclusive_zone (struct Draw_surface *surface)
{
	struct App *app = surface->output->app;
	if ( app->exclusive_zone == 1 ) switch (app->anchor)
	{
		case ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM:
		case ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP:
			return surface->dimensions.h;

		case ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT:
		case ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT:
			return surface->dimensions.w;

		default:
			return 0;
	}
	else
		return surface->output->app->exclusive_zone;
}


bool create_surface (struct Draw_output *output)
{
	struct App *app = output->app;
	printlog(app, 1, "[surface] Creating surface: global_name=%d\n", output->global_name);

	struct Draw_surface *surface = calloc(1, sizeof(struct Draw_surface));
	if ( surface == NULL )
	{
		printlog(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	output->surface             = surface;
	surface->dimensions         = app->dimensions;
	surface->output             = output;
	surface->background_surface = NULL;
	surface->layer_surface      = NULL;
	surface->configured         = false;
	surface->font_description   = pango_font_description_from_string(app->font_pattern);

	surface->background_surface = wl_compositor_create_surface(app->compositor);
	surface->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
					app->layer_shell, surface->background_surface,
					output->wl_output, app->layer,
					app->namespace);

    /* Set up layer surface */
	zwlr_layer_surface_v1_add_listener(surface->layer_surface,
			&layer_surface_listener, surface);

	zwlr_layer_surface_v1_set_size(surface->layer_surface,
			surface->dimensions.w, surface->dimensions.h);
	zwlr_layer_surface_v1_set_anchor(surface->layer_surface, app->anchor);
	zwlr_layer_surface_v1_set_margin(surface->layer_surface,
			app->margin_top, app->margin_right,
			app->margin_bottom, app->margin_left);
	zwlr_layer_surface_v1_set_exclusive_zone(surface->layer_surface,
			get_exclusive_zone(surface));
	if (! app->input)
	{
		struct wl_region *region = wl_compositor_create_region(app->compositor);
		wl_surface_set_input_region(surface->background_surface, region);
		wl_region_destroy(region);
	}

	wl_surface_commit(surface->background_surface);
	return true;
}

void destroy_surface (struct Draw_surface *surface)
{
	if ( surface == NULL )
		return;
	if ( surface->output != NULL )
		surface->output->surface = NULL;
	if ( surface->layer_surface != NULL )
		zwlr_layer_surface_v1_destroy(surface->layer_surface);
	if ( surface->background_surface != NULL )
		wl_surface_destroy(surface->background_surface);
	finish_buffer(&surface->background_buffers[0]);
	finish_buffer(&surface->background_buffers[1]);
	free(surface);
}

void update (struct App *app)
{
	printlog(app, 1, "[surface] Updating\n");
	struct Draw_output *op, *tmp;
	wl_list_for_each_safe(op, tmp, &app->outputs, link)
		if ( op->surface != NULL )
		{
			render_background_frame(op->surface);
			wl_surface_commit(op->surface->background_surface);
		}
}

