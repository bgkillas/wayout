wayout(1) ["Version 0.1.1" ["Version 0.1.1"]]

# NAME
wayout - Output text to a wayland desktop widget


# DESCRIPTION
Wayout takes text from standard input and outputs it to a desktop-widget on Wayland desktops. Periodic updates are
supported (e.g. newline separated input or any other delimiter of choice). The desktop widget can
be shown either on top (OSD-like functionality) or below other windows.

A Wayland compositor must implement the Layer-Shell and XDG-Output for wayout
to work.

# OPTIONS
*-h*, *--help*
	Display help message and exit.

*-V*, *--version*
	Display version and exit.

*-v*, *--verbose*
	Increase verbosity of logging.

*--background-colour* <colour>
	The background colour of the clock. The default is "#FFFFFF".

*--border-colour* <colour>
	The border colour of the clock. The default is "#000000".

*--border-size* <size>, *--border-size* <top> <right> <bottom> <left>
	Set the border size. Requires either one argument, to set all border sizes,
	or four arguments, to set the border sizes individually. The default border
	size is 0 (no border).

*--corner-radius* <radius>, *--corner-radius* <top-left> <top-right> <bottom-right> <bottom-left>
	The radius of the corners. Expects either one argument, to set the radius
	of all corners, or four arguments, to set the individual corner radii. If
	set to sufficiently high values, wlclock will be completely round. Set to 0
	to disable corner roundness. The default radius is 0.

*--exclusive-zone* <option>
	Exclusive zone of the surface. Can be "true", "false" or "stationary". If
	"true", the compositor is informed that it should not obstruct the surface
	of wlclock with other surfaces. If "false", the compositor is informed that
	it can place other surfaces over or under the surface of wlclock. If
	"stationary", the compositor is informed that the surface of wlclock should
	not be moved to accommodate other layer shell surfaces and that it can
	freely place other surfaces over or under the surface of wlclock. The
	default is "stationary". The exact implementation is compositor dependant;
	A compositor may choose to ignore the requested exclusive zone of surfaces.

*--layer* <layer>
	Layer of the layer surface. Can be "overlay", "top", "bottom" or
	"background". Typically, "bottom" and "background" will be underneath
	regular windows, while "top" and "overlay" will be above them, however
	the exact placement on the z axis depends on the implementation of the
	compositor. The default layer is "bottom".

*--margin* <size>, *--margin* <top> <right> <bottom> <left>
	Set the margin. Requires either one argument, to set all margins, or four
	arguments, to set the margins individually. The default margin is 0.

*--namespace* <namespace>
	The namespace of the layers surface. Some compositors may treat layer
	surfaces differently based on their namespace. The default is "wayout".

*-input*
	If this flag is set, wayout will ask the compositor to send it
	input events; Otherwise (the default) mouse events pass through wayout to windows
	beneath it.

*--output* <output name>
	Name of the output on which wlclock should be displayed. Output names
	are compositor dependant. If set to "all" or "\*", wlclock will be displayed
	on all outputs, which is the default behaviour.

*--position* <position>
	Set the position of the clock. Can be "center", "top", "right", "bottom",
	"left", "top-right", "top-left", "bottom-right" and "bottom-left". Defaults
	to "center".

*--width* <size>
	The width of the widget, in pixels with borders. The default width is 320

*--height* <size>
	The height of the widget, in pixels with borders. The default height is 240

*--font* <font>
	Font pattern specification (e.g. Monospace 23)

*--no-wrap*
	Disable wordwrap

*-l*, *--feed-line*
	Update the text periodically and treat each line of the input as an update

*-p*, *--feed-par*
	Update the text periodically and treat each paragraph (parts separated by an empty line) of the input as an update

*--feed-delimiter*
	Update the text periodically, use a custom delimiter. If the input line corresponds to the delimiter, an update is triggered.

*-i*, *--interval* <milliseconds>
	The update interval in milliseconds (only used with the feed options).

# COLOURS
wayout can parse hex code colours and read RGBA values directly.

The supported formats are "#RRGGBB", "#RRGGBBAA", "0xRRGGBB" and "0xRRGGBBAA"
for hex code colours and "rgb(rrr,ggg,bbb)" and "rgba(rrr,ggg,bbb,aaa)" for RGB
colours.

# MARKUP
wayout supports the Pango Text Attribute Markup Language to specify colours and markup within the text itself.
See https://docs.huihoo.com/api/gtk/2.6/pango/PangoMarkupFormat.html

# AUTHORS
Maarten van Gompel <proycon@anaproy.nl>
Leon Henrik Plickat <leonhenrik.plickat@stud.uni-goettingen.de> (wlclock)


