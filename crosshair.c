#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdatomic.h>

#define LINE_WIDTH 2

/* global vars */

Display
*display = NULL;

Window
overlay_window;

int
screen_width;

int screen_height;

atomic_int
should_exit = 0;

/* funcs */

void
cleanup(int sig) {
    should_exit = 1;
}

void
register_global_hotkey() {
    KeyCode q_key = XKeysymToKeycode(display, XStringToKeysym("Q"));
    XGrabKey(display, q_key, ControlMask, DefaultRootWindow(display),
             True, GrabModeAsync, GrabModeAsync);
}

void
check_global_hotkey(XEvent *event) {
    if (event->type == KeyPress) {
        KeySym keysym = XLookupKeysym(&event->xkey, 0);
        
        if ((event->xkey.state & ControlMask) && 
            (keysym == XStringToKeysym("q") || keysym == XStringToKeysym("Q"))) {
            should_exit = 1;
        }
    }
}

void
draw_red_crosshair() {
    GC gc = XCreateGC(display, overlay_window, 0, NULL);
    
    XColor color;
    Colormap colormap = DefaultColormap(display, DefaultScreen(display));
    XParseColor(display, colormap, "red", &color);
    XAllocColor(display, colormap, &color);
    XSetForeground(display, gc, color.pixel);
    
    XSetLineAttributes(display, gc, LINE_WIDTH, LineSolid, CapRound, JoinRound);

    XDrawLine(display, overlay_window, gc,
              screen_width/2, 0,
              screen_width/2, screen_height);

    XDrawLine(display, overlay_window, gc,
              0, screen_height/2,
              screen_width, screen_height/2);

    XFreeGC(display, gc);
    XFlush(display);
}

void
create_overlay_window() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    screen_width = DisplayWidth(display, screen);
    screen_height = DisplayHeight(display, screen);

    printf("Screen size: %dx%d\n", screen_width, screen_height);

    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    attrs.background_pixel = 0;
    attrs.event_mask = ExposureMask | KeyPressMask;

    overlay_window = XCreateWindow(display, root,
                                  0, 0, screen_width, screen_height, 0,
                                  CopyFromParent, InputOutput, CopyFromParent,
                                  CWOverrideRedirect | CWBackPixel | CWEventMask, 
                                  &attrs);

    Pixmap mask = XCreatePixmap(display, overlay_window, screen_width, screen_height, 1);
    GC mask_gc = XCreateGC(display, mask, 0, NULL);

    XSetForeground(display, mask_gc, 0);
    XFillRectangle(display, mask, mask_gc, 0, 0, screen_width, screen_height);

    XSetForeground(display, mask_gc, 1);
    XFillRectangle(display, mask, mask_gc,
                   screen_width/2 - LINE_WIDTH/2, 0, LINE_WIDTH, screen_height);
    XFillRectangle(display, mask, mask_gc,
                   0, screen_height/2 - LINE_WIDTH/2, screen_width, LINE_WIDTH);

    XShapeCombineMask(display, overlay_window, ShapeBounding, 0, 0, mask, ShapeSet);

    XFreePixmap(display, mask);
    XFreeGC(display, mask_gc);

    XMapWindow(display, overlay_window);

    draw_red_crosshair();

    XFlush(display);
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    printf("Starting crosshair overlay...\n");
    create_overlay_window();
    register_global_hotkey();

    printf("Crosshair overlay started\n");
    printf("Center coordinates: %dx%d\n", screen_width/2, screen_height/2);
    printf("Press Ctrl+Q anywhere to exit\n");

    XEvent event;
    while (!should_exit) {
        if (XPending(display)) {
            XNextEvent(display, &event);
            
            if (event.type == Expose) {
                draw_red_crosshair();
            } else if (event.type == KeyPress) {
                check_global_hotkey(&event);
            }
        } else {
            usleep(10000);
        }
    }

    if (display) {
        XDestroyWindow(display, overlay_window);
        XCloseDisplay(display);
        printf("Cleanup completed. Exiting...\n");
    }

    return 0;
}
