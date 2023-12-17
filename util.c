#include "util.h"
#include <X11/extensions/Xrandr.h>
#include <stdio.h>


void getResolution(Display *display,int *width, int *height,int *x, int *y){
    Window root = DefaultRootWindow(display);
    XWindowAttributes windowAttrs;
    XGetWindowAttributes(display, root, &windowAttrs);

    int rootX, rootY, winX, winY;
    Window child;
    unsigned int mask;
    XQueryPointer(display, root, &child, &child, &rootX, &rootY, &winX, &winY, &mask);

    // Query the screen information using XRandR
    XRRScreenResources *res = XRRGetScreenResources(display, root);

    for (int i = 0; i < res->noutput; i++) {
        XRROutputInfo *outputInfo = XRRGetOutputInfo(display, res, res->outputs[i]);
        if (outputInfo->crtc != None) {
            XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(display, res, outputInfo->crtc);
            if (rootX >= crtcInfo->x && rootX < crtcInfo->x + crtcInfo->width &&
                rootY >= crtcInfo->y && rootY < crtcInfo->y + crtcInfo->height) {
                *width =  crtcInfo->width;
                *height =  crtcInfo->height;
                *x = crtcInfo->x ;
                *y = crtcInfo->y ;
            }
            XRRFreeCrtcInfo(crtcInfo);
        }
        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(res);
}
