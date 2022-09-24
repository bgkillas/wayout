#include<errno.h>
#include<getopt.h>
#include<poll.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#ifdef HANDLE_SIGNALS
#include<sys/signalfd.h>
#include<signal.h>
#endif
#include <sys/timerfd.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"wayout.h"
#include"misc.h"
#include"output.h"
#include"surface.h"
#include"colour.h"

#define BUFFERSIZE 65536

static void registry_handle_global (void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct App *app = (struct App *)data;

	if (! strcmp(interface, wl_compositor_interface.name))
	{
		printlog(app, 2, "[main] Get wl_compositor.\n");
		app->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	}
	if (! strcmp(interface, wl_subcompositor_interface.name))
	{
		printlog(app, 2, "[main] Get wl_subcompositor.\n");
		app->subcompositor = wl_registry_bind(registry, name,
				&wl_subcompositor_interface, 1);
	}
	else if (! strcmp(interface, wl_shm_interface.name))
	{
		printlog(app, 2, "[main] Get wl_shm.\n");
		app->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
	else if (! strcmp(interface, zwlr_layer_shell_v1_interface.name))
	{
		printlog(app, 2, "[main] Get zwlr_layer_shell_v1.\n");
		app->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
	}
	else if (! strcmp(interface, zxdg_output_manager_v1_interface.name))
	{
		printlog(app, 2, "[main] Get zxdg_output_manager_v1.\n");
		app->xdg_output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 3);
	}
	else if (! strcmp(interface, wl_output_interface.name))
	{
		if (! create_output(data, registry, name, interface, version))
			goto error;
	}

	return;
error:
	app->loop = false;
	app->ret  = EXIT_FAILURE;
}

static void registry_handle_global_remove (void *data,
		struct wl_registry *registry, uint32_t name)
{
	struct App *app = (struct App *)data;
	printlog(app, 1, "[main] Global remove.\n");
	destroy_output(get_output_from_global_name(app, name));
}

static const struct wl_registry_listener registry_listener = {
	.global        = registry_handle_global,
	.global_remove = registry_handle_global_remove
};

/* Helper function for capability support error message. */
static bool capability_test (void *ptr, const char *name)
{
	if ( ptr != NULL )
		return true;
	printlog(NULL, 0, "ERROR: Wayland compositor does not support %s.\n", name);
	return false;
}

static bool init_wayland (struct App *app)
{
	printlog(app, 1, "[main] Init Wayland.\n");

	/* Connect to Wayland server. */
	printlog(app, 2, "[main] Connecting to server.\n");
	if ( NULL == (app->display = wl_display_connect(NULL)) )
	{
		printlog(NULL, 0, "ERROR: Can not connect to a Wayland server.\n");
		return false;
	}

	/* Get registry and add listeners. */
	printlog(app, 2, "[main] Get wl_registry.\n");
	app->registry = wl_display_get_registry(app->display);

	wl_registry_add_listener(app->registry, &registry_listener, app);

	/* Allow registry listeners to catch up. */
	//TODO use sync instead
	if ( wl_display_roundtrip(app->display) == -1 )
	{
		printlog(NULL, 0, "ERROR: Roundtrip failed.\n");
		return false;
	}

	/* Testing compatibilities. */
	if (! capability_test(app->compositor, "wl_compositor"))
		return false;
	if (! capability_test(app->shm, "wl_shm"))
		return false;
	if (! capability_test(app->layer_shell, "zwlr_layer_shell"))
		return false;
	if (! capability_test(app->xdg_output_manager, "xdg_output_manager"))
		return false;

	printlog(app, 2, "[main] Catching up on output configuration.\n");
	struct Draw_output *op;
	wl_list_for_each(op, &app->outputs, link)
		if ( ! op->configured && ! configure_output(op) )
			return false;

	return true;
}

/* Finish him! */
static void finish_wayland (struct App *app)
{
	if ( app->display == NULL )
		return;

	printlog(app, 1, "[main] Finish Wayland.\n");

	destroy_all_outputs(app);

	printlog(app, 2, "[main] Destroying Wayland objects.\n");
	if ( app->layer_shell != NULL )
		zwlr_layer_shell_v1_destroy(app->layer_shell);
	if ( app->compositor != NULL )
		wl_compositor_destroy(app->compositor);
	if ( app->shm != NULL )
		wl_shm_destroy(app->shm);
	if ( app->registry != NULL )
		wl_registry_destroy(app->registry);

	printlog(app, 2, "[main] Disconnecting from server.\n");
	wl_display_disconnect(app->display);
}

static bool handle_command_flags (struct App *app, int argc, char *argv[])
{

	const char *usage =
		"Usage: wayout [options]\n"
		"\n"
		"  -h, --help                      Show this help text.\n"
		"  -v, --verbose                   Increase verbosity of output.\n"
		"  -V, --version                   Show the version.\n"
		"      --background-colour [#rgba] Background colour.\n"
		"      --border-colour [#rgba]     Border colour.\n"
		"      --border-size [px]          Size of the border.\n"
		"      --text-colour [#rgba]       Colour of the text.\n"
		"      --corner-radius [px]        Corner radii.\n"
		"      --exclusive-zone            Exclusive zone of the layer surface.\n"
		"      --layer [top,bottom,overlay,background]\n"
        "                                  Layer to place the widget\n"
		"      --margin                    Directional margins.\n"
		"      --namespace                 Namespace of the layer surface.\n"
		"      --input                     Do not let input pass trough the layer surface.\n"
		"      --output                    The output on which the widget will be displayed.\n"
		"      --position                  Set the position of the widget.\n"
		"      --width [px]                Set the width of the widget.\n"
		"      --height [px]               Set the height of the widget.\n"
		"      --font [font pattern]       Font pattern (e.g. Monospace 23)\n"
		"      --center                    Center alignment (horizontally)\n"
		"  -w, --no-wrap                   Disable wordwrap\n"
		"  -l, --feed-line                 Each line delimits the input\n"
		"  -p, --feed-par                  Empty lines delimit the input\n"
		"  -d, --feed-delimiter [line]     A custom delimiter delimits the input\n"
		"  -i, --interval [ms]             Poll interval to check for new input\n"
		"\n";

	int i;
	for (i = 1; argv[i]; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i],"--help")) {
			fputs(usage, stderr);
			app->ret = EXIT_SUCCESS;
			return false;
		} else if (!strcmp(argv[i], "-v") || !strcmp(argv[i],"--verbose")) {
			app->verbosity++;
		} else if (!strcmp(argv[i], "-V") || !strcmp(argv[i],"--version")) {
			fputs("wayout version " VERSION "\n", stderr);
			app->ret = EXIT_SUCCESS;
			return false;
		} else if (!strcmp(argv[i],"--width")) {
			if (i + 1 >= argc) goto error;
			app->dimensions.w    = atoi(argv[++i]);
		} else if (!strcmp(argv[i],"--height")) {
			if (i + 1 >= argc) goto error;
			app->dimensions.h    = atoi(argv[++i]);
		} else if (!strcmp(argv[i],"--position")) {
			if (i + 1 >= argc) goto error;
			if (! strcmp(argv[i+1], "center"))
				app->anchor = 0;
			else if (! strcmp(argv[i+1], "top"))
				app->anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
			else if (! strcmp(argv[i+1], "right"))
				app->anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
			else if (! strcmp(argv[i+1], "bottom"))
				app->anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
			else if (! strcmp(argv[i+1], "left"))
				app->anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
			else if (! strcmp(argv[i+1], "top-left"))
				app->anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
			else if (! strcmp(argv[i+1], "top-right"))
				app->anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
			else if (! strcmp(argv[i+1], "bottom-left"))
				app->anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
			else if (! strcmp(argv[i+1], "bottom-right"))
				app->anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
			else
			{
				printlog(NULL, 0, "ERROR: Unrecognized position \"%s\".\n"
						"INFO: Possible positisions are 'center', "
						"'top', 'right', 'bottom', 'left', "
						"'top-right', 'top-left', 'bottom-right', 'bottom-left'.\n",
						optarg);
				return false;
			}
			i++;
		} else if (!strcmp(argv[i],"--background-colour") || !strcmp(argv[i],"--background-color")) {
			if (i + 1 >= argc) goto error;
			if (! colour_from_string(&app->background_colour, argv[++i]))
				return false;
		} else if (!strcmp(argv[i],"--border-colour") || !strcmp(argv[i],"--border-color")) {
			if (i + 1 >= argc) goto error;
			if (! colour_from_string(&app->border_colour, argv[++i]))
				return false;
		} else if (!strcmp(argv[i],"--border-size")) {
			if (i + 1 >= argc) goto error;
			app->border_top = app->border_right =
				app->border_bottom = app->border_left =
				atoi(argv[++i]);
			if ( app->border_top < 0 || app->border_right < 0
					|| app->border_bottom < 0 || app->border_left < 0 )
			{
				printlog(NULL, 0, "ERROR: Borders may not be smaller than zero.\n");
				return false;
			}
		} else if (!strcmp(argv[i],"--text-colour") || !strcmp(argv[i],"--text-color") || !strcmp(argv[i],"--foreground-colour") || !strcmp(argv[i],"--foreground-color")) {
			if (i + 1 >= argc) goto error;
			if (! colour_from_string(&app->text_colour, argv[++i]))
				return false;
		} else if (!strcmp(argv[i],"--exclusive-zone")) {
			if (i + 1 >= argc) goto error;
			if (is_boolean_true(argv[i+1]))
				app->exclusive_zone = 1;
			else if (is_boolean_false(argv[i+1]))
				app->exclusive_zone = 0;
			else if (! strcmp(argv[i+1], "stationary"))
				app->exclusive_zone = -1;
			else
			{
				printlog(NULL, 0, "ERROR: Unrecognized exclusive zone option \"%s\".\n"
						"INFO: Possible options are 'true', "
						"'false' and 'stationary'.\n", argv[i+1]);
				return false;
			}
			i++;
		} else if (!strcmp(argv[i],"--layer")) {
			if (i + 1 >= argc) goto error;
			if (! strcmp(argv[i+1], "overlay"))
				app->layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
			else if (! strcmp(argv[i+1], "top"))
				app->layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
			else if (! strcmp(argv[i+1], "bottom"))
				app->layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
			else if (! strcmp(argv[i+1], "background"))
				app->layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
			else
			{
				printlog(NULL, 0, "ERROR: Unrecognized layer \"%s\".\n"
						"INFO: Possible layers are 'overlay', "
						"'top', 'bottom', and 'background'.\n", argv[i+1]);
				return false;
			}
			i++;
		} else if (!strcmp(argv[i],"--margin")) {
			if (i + 1 >= argc) goto error;
			app->margin_top = app->margin_right =
				app->margin_bottom = app->margin_left =
				atoi(argv[++i]);
			if ( app->margin_top < 0 || app->margin_right < 0
					|| app->margin_bottom < 0 || app->margin_left < 0 )
			{
				printlog(NULL, 0, "ERROR: Margins may not be smaller than zero.\n");
				return false;
			}
		} else if (!strcmp(argv[i],"--margin-top")) {
			if (i + 1 >= argc) goto error;
			app->margin_top    = atoi(argv[++i]);
		} else if (!strcmp(argv[i],"--margin-bottom")) {
			if (i + 1 >= argc) goto error;
			app->margin_bottom    = atoi(argv[++i]);
		} else if (!strcmp(argv[i],"--margin-left")) {
			if (i + 1 >= argc) goto error;
			app->margin_left    = atoi(argv[++i]);
		} else if (!strcmp(argv[i],"--margin-right")) {
			if (i + 1 >= argc) goto error;
			app->margin_right    = atoi(argv[++i]);
		} else if (!strcmp(argv[i],"--namespace")) {
			if (i + 1 >= argc) goto error;
			set_string(&app->namespace, argv[++i]);
		} else if (!strcmp(argv[i],"--input")) {
			app->input = true;
		} else if (!strcmp(argv[i],"--output")) {
			if (i + 1 >= argc) goto error;
			if ( ! strcmp("all", argv[i+1]) || ! strcmp("*", argv[i+1]) ) {
				free_if_set(app->output);
				i++;
			} else
				set_string(&app->output, argv[++i]);
		} else if (!strcmp(argv[i],"--corner-radius")) {
			if (i + 1 >= argc) goto error;
			app->radius_top_left = app->radius_top_right =
				app->radius_bottom_right = app->radius_bottom_left =
				atoi(argv[++i]);
			if ( app->radius_top_left < 0 || app->radius_top_right < 0
					|| app->radius_bottom_right < 0 || app->radius_bottom_left < 0 )
			{
				printlog(NULL, 0, "ERROR: Radii may not be smaller than zero.\n");
				return false;
			}
		} else if (!strcmp(argv[i],"-l") || !strcmp(argv[i],"--feed-line")) {
            app->feed = true;
            app->delimiter = "";
		} else if (!strcmp(argv[i],"-p") || !strcmp(argv[i],"--feed-par")) {
            app->feed = true;
            app->delimiter = "\n";
		} else if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--feed-delimiter")) {
			if (i + 1 >= argc) goto error;
            app->feed = true;
            app->delimiter = argv[++i];
		} else if (!strcmp(argv[i],"-i") || !strcmp(argv[i],"--interval")) {
			if (i + 1 >= argc) goto error;
            app->interval = atoi(argv[++i]);
		} else if (!strcmp(argv[i],"--font")) {
			if (i + 1 >= argc) goto error;
			app->font_pattern = strdup(argv[++i]);
		} else if (!strcmp(argv[i],"--fontsize")) {
			//added for backward compatibility
			if (i + 1 >= argc) goto error;
			char * target = malloc(strlen(app->font_pattern) + 10);
			sprintf(target, "%s %s", app->font_pattern, argv[++i]);
			app->font_pattern = target;
		} else if (!strcmp(argv[i],"--textalign")) {
			//added for backward compatibility
			if (i + 1 >= argc) goto error;
			if (!strcmp(argv[++i],"center")) {
				app->center = true;
			}
		} else if (!strcmp(argv[i],"-w") || !strcmp(argv[i],"--no-wrap")) {
			app->wordwrap = false;
		} else if (!strcmp(argv[i],"--center")) {
			app->center = true;
		} else {
			printlog(app, 0, "Invalid parameter: %s", argv[i]);
			return false;
		}
	}
	return true;
	error:
		printlog(app, 0, "Parameter required");
		return false;
}



/* Timeout until next update */

static void app_run (struct App *app)
{
	printlog(app, 1, "[main] Starting loop.\n");
	app->ret = EXIT_SUCCESS;

	struct pollfd fds[4] = { 0 };
	size_t wayland_fd = 0;
	size_t stdin_fd = 1;
	size_t timer_fd = 2;
	size_t signal_fd = 3;
	size_t fd_count = 4;

	fds[wayland_fd].events = POLLIN;
	if ( -1 ==  (fds[wayland_fd].fd = wl_display_get_fd(app->display)) )
	{
		printlog(NULL, 0, "ERROR: Unable to open Wayland display fd.\n");
		goto error;
	}

	fds[stdin_fd].events = POLLIN;
	fds[stdin_fd].fd = STDIN_FILENO;
	if ( 0 > (fds[stdin_fd].fd = STDIN_FILENO)) {
		printlog(NULL, 0, "ERROR: Unable to open stdin fd.\n");
		goto error;
	}

	if (app->feed) {
		struct itimerspec timer_value;
		fds[timer_fd].events = POLLIN;
		if ( 0 > (fds[timer_fd].fd = timerfd_create(CLOCK_MONOTONIC, 0))) {
			printlog(NULL, 0, "ERROR: Unable to open timer fd.\n");
			goto error;
		}
		memset(&timer_value, 0, sizeof(timer_value));
		timer_value.it_value.tv_sec = timer_value.it_interval.tv_sec = floor((double) app->interval / 1000.0);
		timer_value.it_value.tv_nsec = timer_value.it_interval.tv_nsec = floor(app->interval % 1000) * 1000000; //ms to ns

		if (timerfd_settime(fds[timer_fd].fd, 0, &timer_value, NULL) < 0) {
			printlog(NULL, 0, "ERROR: Unable to start timer.\n");
			goto error;
		}
	} else {
		fds[timer_fd].fd = -1;
		fds[timer_fd].events = -1;
		fds[timer_fd].revents = -1;
	}


#ifdef HANDLE_SIGNALS
	sigset_t mask;
	struct signalfd_siginfo fdsi;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	if ( sigprocmask(SIG_BLOCK, &mask, NULL) == -1 )
	{
		printlog(NULL, 0, "ERROR: sigprocmask() failed.\n");
		goto error;
	}
	fds[signal_fd].events = POLLIN;
	if ( -1 ==  (fds[signal_fd].fd = signalfd(-1, &mask, 0)) )
	{
		printlog(NULL, 0, "ERROR: Unable to open signal fd.\n"
				"ERROR: signalfd: %s\n", strerror(errno));
		goto error;
	}
#endif

	char buffer[BUFFERSIZE];
	char * bufferhead = (char*) &buffer;
	bool first_update = true; //first update is forced

	while (app->loop)
	{
		/* Flush pending Wayland events/requests. */
		int ret = 1;
		while ( ret > 0 )
		{
			ret = wl_display_dispatch_pending(app->display);
			wl_display_flush(app->display);
		}
		if ( ret < 0 )
		{
			printlog(NULL, 0, "ERROR: wl_display_dispatch_pending: %s\n",
					strerror(errno));
			app->ret = EXIT_FAILURE;
			goto exit;
		}

		printlog(app, 3, "Polling...\n");
		ret = poll(fds, fd_count, !app->ready || app->require_update ? 500 : -1); //blocking once app is ready and we have text
		if ( ret < 0 )
		{
			printlog(NULL, 0, "ERROR: poll: %s\n", strerror(errno));
			continue;
		}
		printlog(app, 3, "Polled %d, wayland=%d, stdin=%d, signal=%d, timer=%d \n",ret, fds[wayland_fd].revents, fds[stdin_fd].revents, fds[signal_fd].revents, fds[timer_fd].revents);

		/* Wayland events */
		if ( fds[wayland_fd].revents & POLLIN && wl_display_dispatch(app->display) == -1 )
		{
			printlog(NULL, 0, "ERROR: wl_display_flush: %s\n", strerror(errno));
			goto error;
		}
		if ( fds[wayland_fd].revents & POLLOUT && wl_display_flush(app->display) == -1 )
		{
			printlog(NULL, 0, "ERROR: wl_display_flush: %s\n", strerror(errno));
			goto error;
		}

		size_t line_size;
		char *line = NULL;
		bool flushbuffer = false;

		if ( fds[stdin_fd].revents & POLLIN)
		{
			printlog(app, 2, "Processing stdin\n");
			if ( getline(&line, &line_size, stdin) != -1 ) {
				printlog(app, 2, "Read line (size=%d)\n", line_size);
				if (app->feed && strcmp(app->delimiter,"") == 0) {
					if (app->text != NULL) free(app->text);
					app->text = "t";
					app->require_update = true;
				} else if (app->feed && strcmp(app->delimiter, line) == 0) {
					flushbuffer = true;
				} else {
					if ((bufferhead - buffer) + strlen(line) >= BUFFERSIZE) {
						printlog(app, 2, "Buffer size exceeded.. ignoring line\n");
					} else {
						strncpy(bufferhead, line, strlen(line));
						bufferhead += strlen(line);
					}
				}
				if (feof(stdin)) flushbuffer = true; //not sure this can actually happen here
			} else {
				printlog(app, 2, "No line to get\n");
				flushbuffer = true;
			}

		}

		if ((fds[stdin_fd].revents & POLLHUP))
		{
			//input pipe got closed, process all remaining input
			printlog(app, 2, "Input pipe got closed\n");
			while ( fgets(bufferhead, BUFFERSIZE - strlen(bufferhead), stdin) ) {
				bufferhead += strlen(bufferhead);
			}
			if (bufferhead != buffer) flushbuffer = true;

			//stop reading stdin
			fds[stdin_fd].events = 0;
			fds[stdin_fd].revents = 0;
			fds[stdin_fd].fd = -1;
		}

		if ( fds[timer_fd].revents & POLLIN)
		{
			printlog(app, 3, "timer tick\n");
			uint64_t elapsed = 0;
			read(fds[timer_fd].fd, &elapsed, sizeof(elapsed));
		}

		if ((flushbuffer) && (bufferhead != buffer)) {
			printlog(app, 2, "Flushing buffer (size %d)\n", bufferhead - buffer);
			if (app->text != NULL) free(app->text);
			*bufferhead = 0;
			app->text = strdup(buffer);
			bufferhead = (char*) &buffer;
			*bufferhead = 0;
			app->require_update = true;
		}

#ifdef HANDLE_SIGNALS
		/* Signal events. */
		if ( fds[signal_fd].revents & POLLIN )
		{
			printlog(app, 1, "Got signal");
			if ( read(fds[signal_fd].fd, &fdsi, sizeof(struct signalfd_siginfo))
					!= sizeof(struct signalfd_siginfo) )
			{
				printlog(NULL, 0, "ERROR: Can not read signal info.\n");
				goto error;
			}

			if ( fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGQUIT || fdsi.ssi_signo == SIGTERM )
			{
				printlog(app, 1, "[main] Received SIGINT, SIGQUIT or SIGTERM; Exiting.\n");
				goto exit;
			}
			else if ( fdsi.ssi_signo == SIGUSR1 || fdsi.ssi_signo == SIGUSR2 )
			{
				printlog(app, 1, "[main] Received SIGUSR; Forcing update.\n");
				update(app);
			}

		}

		if ((first_update) || (fds[timer_fd].revents & POLLIN)) {
			if (app->ready) {
				if (app->require_update) {
					printlog(app, 1, "Calling update.\n");
					update(app);
					app->require_update = false;
					first_update = false;
				}
			} else {
				printlog(app, 3, "App not ready yet.\n");
			}
		}
#endif
	}

	return;
error:
	app->ret = EXIT_FAILURE;
#ifdef HANDLE_SIGNALS
exit:
	if ( fds[signal_fd].fd != -1 )
		close(fds[signal_fd].fd);
#endif
	if ( fds[wayland_fd].fd != -1 )
		close(fds[wayland_fd].fd);
	return;
}

int main (int argc, char *argv[])
{
	struct App app = { 0 };
	wl_list_init(&app.outputs);
	app.ret = EXIT_FAILURE;
	app.loop = true;
	app.verbosity = 0;

	app.exclusive_zone = -1;
	app.input = false;
	app.snap = false;
	app.layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
	app.anchor = 0; /* Center */
	app.interval = 1000;
	app.wordwrap = true;
	app.center = false;
	app.font_pattern = "Monospace 26";
	app.delimiter = "";
	set_string(&app.namespace, "wayout");
	app.border_bottom = app.border_top
		= app.border_left = app.border_right = 0;
	app.radius_bottom_left = app.radius_bottom_right
		= app.radius_top_left = app.radius_top_right = 0;
	app.margin_bottom = app.margin_top
		= app.margin_left = app.margin_right = 0;
	app.dimensions.w = 320;
	app.dimensions.h = 240;
	app.require_update = true;
	colour_from_string(&app.background_colour, "#00000000");
	colour_from_string(&app.border_colour,     "#000000");
	colour_from_string(&app.text_colour,      "#ffffff");

	if (! handle_command_flags(&app, argc, argv))
		goto exit;


	printlog(&app, 1, "[main] wayout: version=%s\n[main] w=%d h=%d font=%s\n",
			VERSION,
			app.dimensions.w, app.dimensions.h, app.font_pattern);

	if (! init_wayland(&app))
		goto exit;

	app_run(&app);

exit:
	finish_wayland(&app);
	free_if_set(app.output);
	free_if_set(app.namespace);
	return app.ret;
}

