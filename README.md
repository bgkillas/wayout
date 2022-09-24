# wayout

Wayout takes text from standard input and outputs it to a desktop-widget on Wayland desktops. Periodic updates are
supported; e.g. newline separated input or any other delimiter of choice. We call this a *feed*. The desktop widget can
be shown either on top (OSD-like functionality) or below other windows.

A Wayland compositor must implement the Layer-Shell and XDG-Output for wayout
to work.

## Features

* Allow updating from standard input at a regular interval; custom delimiters
* Configurable colours/border/position/fonts
* Supports the [pango markup language](https://docs.huihoo.com/api/gtk/2.6/pango/PangoMarkupFormat.html) for text
	markup/colours.

## Building

wayout depends on Wayland, Wayland protocols, Cairo and Pango.

To build this program you will need a C compiler, the meson & ninja build system
and `scdoc` to generate the manpage.

    git clone https://git.sr.ht/~proycon/wayout
    cd wayout
    meson build
    ninja -C build
    sudo ninja -C build install

## Usage

Static example for a calendar:

```
$ cal | wayout
```

Example to use wayout as a simple digital clock using ``--feed-line``:

```
$ while; do date +%H:%M:%S; sleep 1; done | wayout --feed-line
```

You can use the [pango markup language](https://docs.huihoo.com/api/gtk/2.6/pango/PangoMarkupFormat.html) for text
markup and colours:

```
$ echo "<b>bold</b>\n<span foreground=\"red\">red</span>" | wayout
```

## Troubleshooting

* **Q:** I'm using wayout (without ``--feed`` )from a pipe and the input is not processed.
* **A**: The input is expected to be immediately available when running without ``--feed``, there may be a delay, you may want to use ``--feed-par`` or
	``--feed-line``.

## Contributing

**Contributions are welcome!**  Code contributions can be made as part of the Sxmo project by sending patches to our
mailing list at ~mil/sxmo-devel@lists.sr.ht . See [here](https://sxmo.org/contribute/) for further instructions.

## Licensing

wayout is licensed under the GPLv3.

The contents of the `protocol` directory are licensed differently. Please see
the header of the files for more information.


## Authors

Wayout is for a large part derived from [wlclock](https://git.sr.ht/~leon_plickat/wlclock) by Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
Modifications to this were added by [Sxmo](https://sxmo.org) developers

