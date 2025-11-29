#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define LINE_WIDTH 2

/* global vars */

Display
*g_display = NULL;

Window
g_overlay_window;

int
g_screen_width;

int
g_screen_height;

int
g_should_exit = 0;

/* funcs */

void
cleanup(int sig) {
    g_should_exit = 1;
}

void
register_global_hotkey() {
    KeyCode q_key = XKeysymToKeycode(g_display, XStringToKeysym("Q"));
    XGrabKey(g_display, q_key, ControlMask, DefaultRootWindow(g_display),
             True, GrabModeAsync, GrabModeAsync);
}

void
check_global_hotkey(XEvent *event) {
    if (event->type == KeyPress) {
        KeySym keysym = XLookupKeysym(&event->xkey, 0);

        if ((event->xkey.state & ControlMask) && 
            (keysym == XStringToKeysym("q") || keysym == XStringToKeysym("Q"))) {
            g_should_exit = 1;
        }
    }
}

void
draw_red_crosshair() {
    GC gc = XCreateGC(g_display, g_overlay_window, 0, NULL);

    XColor color;
    Colormap colormap = DefaultColormap(g_display, DefaultScreen(g_display));
    XParseColor(g_display, colormap, "red", &color);
    XAllocColor(g_display, colormap, &color);
    XSetForeground(g_display, gc, color.pixel);

    XSetLineAttributes(g_display, gc, LINE_WIDTH, LineSolid, CapRound, JoinRound);

    XDrawLine(g_display, g_overlay_window, gc,
              g_screen_width/2, 0,
              g_screen_width/2, g_screen_height);

    XDrawLine(g_display, g_overlay_window, gc,
              0, g_screen_height/2,
              g_screen_width, g_screen_height/2);

    XFreeGC(g_display, gc);
    XFlush(g_display);
}

void
create_overlay_window() {
    g_display = XOpenDisplay(NULL);
    if (!g_display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    int screen = DefaultScreen(g_display);
    Window root = RootWindow(g_display, screen);

    g_screen_width = DisplayWidth(g_display, screen);
    g_screen_height = DisplayHeight(g_display, screen);

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = 0;
    attrs.event_mask = ExposureMask | KeyPressMask;

    g_overlay_window = XCreateWindow(g_display, root,
                                  0, 0, g_screen_width, g_screen_height, 0,
                                  CopyFromParent, InputOutput, CopyFromParent,
                                  CWOverrideRedirect | CWBackPixel | CWEventMask,
                                  &attrs);

    /* create mask */
    Pixmap mask = XCreatePixmap(g_display, g_overlay_window, g_screen_width, g_screen_height, 1);
    GC mask_gc = XCreateGC(g_display, mask, 0, NULL);

    /* all windows transparent */
    XSetForeground(g_display, mask_gc, 0);
    XFillRectangle(g_display, mask, mask_gc, 0, 0, g_screen_width, g_screen_height);

    /* crosshair no transparent */
    XSetForeground(g_display, mask_gc, 1);
    XFillRectangle(g_display, mask, mask_gc,
                   g_screen_width/2 - LINE_WIDTH/2, 0, LINE_WIDTH, g_screen_height);
    XFillRectangle(g_display, mask, mask_gc,
                   0, g_screen_height/2 - LINE_WIDTH/2, g_screen_width, LINE_WIDTH);

    /* apply mask to window */
    XShapeCombineMask(g_display, g_overlay_window, ShapeBounding, 0, 0, mask, ShapeSet);

    /* cleanup */
    XFreePixmap(g_display, mask);
    XFreeGC(g_display, mask_gc);

    XMapWindow(g_display, g_overlay_window); /* display window */

    draw_red_crosshair();

    XFlush(g_display); /* apply all */
}

int
main() {
    XEvent event;

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    printf("Starting crosshair overlay...\n");
    create_overlay_window();
    register_global_hotkey();

    printf("Crosshair overlay started\n");
    printf("Screen size: %dx%d\n", g_screen_width, g_screen_height);
    printf("Center coordinates: %dx%d\n", g_screen_width/2, g_screen_height/2);
    printf("Press 'ctrl+q' anywhere to exit\n");

    while (!g_should_exit) {
        if (XPending(g_display)) {
            XNextEvent(g_display, &event);
            
            if (event.type == Expose) {
                draw_red_crosshair();
            } else if (event.type == KeyPress) {
                check_global_hotkey(&event);
            }
        } else {
            usleep(10000);
        }
    }

    if (g_display) {
        XDestroyWindow(g_display, g_overlay_window);
        XCloseDisplay(g_display);
        printf("Cleanup completed. Exiting...\n");
    }

    return 0;
}
