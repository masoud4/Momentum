#include <stdio.h>
#include <X11/Xlib.h>

int main() {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen_count = XScreenCount(display);
    for (int i = 0; i < screen_count; i++) {
        Screen *screen = XScreenOfDisplay(display, i);
        printf("Screen %d:\n", i);
        printf("\tWidth: %d pixels\n", XDisplayWidth(display, i));
        printf("\tHeight: %d pixels\n", XDisplayHeight(display, i));
        printf("\tRoot window: 0x%lx\n", XRootWindowOfScreen(screen));
        printf("\tDefault visual: 0x%lx\n", XDefaultVisualOfScreen(screen));
        printf("\tDefault depth: %d\n", XDefaultDepthOfScreen(screen));
    }

    XCloseDisplay(display);
    return 0;
}
