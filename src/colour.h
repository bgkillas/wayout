#ifndef WLCLOCK_COLOUR_H
#define WLCLOCK_COLOUR_H

#include<stdbool.h>
#include<cairo/cairo.h>

struct Draw_colour
{
	double r, g, b, a;
};

bool colour_from_string (struct Draw_colour *colour, const char *hex);
void colour_set_cairo_source (cairo_t *cairo, struct Draw_colour *colour);
bool colour_is_transparent (struct Draw_colour *colour);

#endif

