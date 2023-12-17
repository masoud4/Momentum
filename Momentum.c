#include "Momentum.h"
 // #include "MDK.h"
#include "X11/X.h"
#include "X11/XKBlib.h"
#include "X11/Xlib.h"
#include "X11/Xproto.h"
#include "X11/Xutil.h"
#include "util.h"
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Monitor monitors[MAX_MONITOR];
/*
    *_NET_WM_STATE_MODAL
    *_NET_WM_STATE_STICKY
    *_NET_WM_STATE_MAXIMIZED_VERT
    *_NET_WM_STATE_MAXIMIZED_HORZ
    *_NET_WM_STATE_SHADED
    *_NET_WM_STATE_SKIP_TASKBAR
    *_NET_WM_STATE_SKIP_PAGER
    *_NET_WM_STATE_HIDDEN
    *_NET_WM_STATE_FULLSCREEN
    *_NET_WM_STATE_ABOVE
    *_NET_WM_STATE_BELOW
    *_NET_WM_STATE_DEMANDS_ATTENTION
    * EWMH
    * https://specifications.freedesktop.org/wm-spec/1.1/x104.html
*/
bool addWindow(Window window, int monitorIndex) {
    // Check if the window is valid and not the root window or null.
    if (WINDOW_IS_NOT_VALID)
        return false;

    // Check if the monitorIndex is within a valid range.
    if (monitorIndex < 0 || monitorIndex >= MAX_MONITOR)
        return false;

    // Allocate memory for the new window entry.
    struct client *newWindowEntry = malloc(sizeof(struct client));

    // Initialize the new window entry.
    newWindowEntry->last = monitors[monitorIndex].clients;
    newWindowEntry->window = window;
    newWindowEntry->identifier = -1;

    // Update the list of clients for the specified monitor.
    monitors[monitorIndex].clients = newWindowEntry;

    // Update the current window for the monitor.
    updateCurrentWindow(window, monitorIndex);

    return true;
}

bool removeWindow(Window window, int monitorIndex) {
    if (WINDOW_IS_NOT_VALID) {
        return false;
    }
    if (monitorIndex < 0 || monitorIndex >= MAX_MONITOR) {
        return false;
    }

    // Check if the window is the panel's window and reset panel attributes.
    if (window == panel.window) {
        panel.window = None;
        panel.width = panel.height = panel.position = 0;
    }

    // Iterate through the clients list for the specified monitor.
    for (struct client **windows = &monitors[monitorIndex].clients;
         *windows && (*windows)->window; windows = &(*windows)->last) {
        if ((*windows)->window == window) {
            // Remove the window entry and update the current window.
            *windows = (*windows)->last;
            updateCurrentWindow(-1, monitorIndex);
            return true;
        }
    }
    return false;
}

void dummyClient(void) {
    for (int i = 0; i < MAX_MONITOR; i++) {
        monitors[i].clients = malloc(sizeof(struct client));
        monitors[i].clients->last = malloc(sizeof(struct client));
        monitors[i].current = monitors[i].clients;
        monitors[i].x_init = 300;
        monitors[i].y_init = 200;
    }
    panel.window = None;
    panel.height = panel.width = 0;
}
 // void showLog(){
 //
 //     init();
 //     window masoud = create_window((const char *)("masoud"), 600,400,100,100,None,None);
 //     
 //     window b2 = create_button((const char *)("exit"),90,20,500,380,masoud) ;
 //     
 //     window t1 = create_textbox((const char *)("t1"),(const char *)(getWindowsDataAsstring),200,200,100,100,masoud) ;
 //     call_back(b2, exit,NULL);
 //   
 // }

char* getWindowsDataAsstring() {
    char *result = malloc(1);  // Start with an empty string
    result[0] = '\0';

    for (int i = 0; i < MAX_MONITOR; i++) {
        Window current = monitors[i].current != NULL
                                 ? monitors[i].current->window
                                 : (Window) -1;
        if (current == None)
            return result;

        // Create a temporary buffer to hold the current line
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%d : { current : %lu,  clients :", i, current);
        strcat(result, buffer);

        if (monitors[i].current->window != NULL) {
            for (struct client *windows = monitors[i].clients;
                 windows && windows->window && windows->last;
                 windows = windows->last) {
                snprintf(buffer, sizeof(buffer), "   %lu : %d", windows->window, windows->identifier);
                strcat(result, buffer);
            }
        }

        strcat(result, "  },\n");
    }

    return result;
}
void getWindowsData() {
    for (int i = 0; i < MAX_MONITOR; i++) {
        Window current = monitors[i].current != (struct client *) NULL
                                 ? monitors[i].current->window
                                 : (Window) -1;
        if (current == None)
            return;
        printf("%d : { current : %lu,  clients :", i, current);
        if (monitors[i].current->window != (Window) NULL) {
            for (struct client *windows = monitors[i].clients;
                 windows && windows->window && windows->last;
                 windows = windows->last) {
                printf("   %lu : %d", windows->window, windows->identifier);
            }
        }
        printf("  },\n");
    }
}

bool updateCurrentWindow(Window window, int monitorIndex) {
    if (WINDOW_IS_NOT_VALID) {
        return false;
    }
    if (monitorIndex < 0 || monitorIndex >= MAX_MONITOR) {
        return false;
    }

    struct client *currentWindow = monitors[monitorIndex].clients;

    // Iterate through the clients list for the specified monitor.
    while (currentWindow && currentWindow->window) {
        if (currentWindow->window == window) {
            monitors[monitorIndex].current = currentWindow;
            return true;
        }
        currentWindow = currentWindow->last;
    }

    // If the specified window was not found, set the current window to the first
    // window.
    monitors[monitorIndex].current = monitors[monitorIndex].clients;
    return false;
}

bool updateWindow(struct client *windows, int Index) {
    if (windows->window == Root || windows->window == 0)
        return false;
    if (Index < 0 || Index >= MAX_MONITOR)
        return false;
    for (struct client **tmp = &monitors[Index].clients; tmp && (*tmp)->window;
         tmp = &(*tmp)->last) {
        if (windows->window == (*tmp)->window) {
            windows->last = (*tmp)->last;
            *tmp = windows;
            return true;
        }
    }
    return false;
}

void setWorkspaceNumber(Display *display, Window window,
                        unsigned long workspace_number) {
    Atom atom_workspace = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
    if (atom_workspace != None) {
        XChangeProperty(display, window, atom_workspace, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *) &workspace_number, 1);
        XSync(display, False);
    } else {
        printf("Failed to get workspace atom\n");
    }
}

bool modifyMonitor(Window window, int Index, int NextIndex) {
    if (window == Root || window == 0)
        return false;
    if (Index < 0 || Index >= MAX_MONITOR || NextIndex < 0 ||
        NextIndex >= MAX_MONITOR)
        return false;

    struct client **tmp;

    for (tmp = &monitors[Index].clients; tmp && (*tmp)->window && (*tmp)->last;
         tmp = &(*tmp)->last) {
        if (window == (*tmp)->window) {
            monitors[NextIndex].clients->last = monitors[NextIndex].clients;
            monitors[NextIndex].clients = *tmp;
            *tmp = (*tmp)->last;
            if (monitors[Index].current->window == window) {
                updateCurrentWindow(-1, Index);
                return true;
            }
            // updateCurrentWindow(window, NextIndex);
            return true;
        }
    }
    return false;
}

void execute(const Arg *arg) {
    if (fork() == 0) {
        setsid();
        execvp(((char **) arg->v)[0], (char **) arg->v);
        fprintf(stderr, "Momentum: Execute %s", ((char **) arg->v)[0]);
        perror(" failed");
        exit(EXIT_SUCCESS);
    }
}

bool initRootWidow(void) {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open X display\n");
        return False;
    }
    XTextProperty wmName;
    const char *wmNameString = "Momentum";
    if (XStringListToTextProperty((char **) &wmNameString, 1, &wmName) == 0) {
        fprintf(stderr, "Failed to set window manager name.\n");
        return 1;
    }

    screen = XScreenOfDisplay(display, 0);
    Root = XRootWindowOfScreen(screen);
    XSetWMName(display, Root, &wmName);
    XSetErrorHandler(OnXError);
    XGrabServer(display);
    XSync(display, False);
    XSelectInput(display, Root,
                 SubstructureRedirectMask | SubstructureNotifyMask);
    XUngrabServer(display);
    grabkeys();
    setWorkspaceNumber(display, Root, (unsigned long) 0);
    MonitorIndex = 1;
    return True;
}

int OnXError(Display *display, XErrorEvent *event) {
    char error_text[MAX_LENGTH];
    XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
    printf("Xlib error: %s\n", error_text);
    printf("  Request code: %d\n", event->request_code);
    printf("  Error code: %d\n", event->error_code);
    printf("  Resource ID: %lu\n", event->resourceid);
    printf("  Serial number: %lu\n", event->serial);
    printf("  Minor opcode: %d\n", event->minor_code);
    printf("  Error type: %d\n", event->type);
    return 0;
}

void grabkeys(void) {
    unsigned int k;
    unsigned int j;
    unsigned int modifiers[] = {0, LockMask, numlockmask, numlockmask | LockMask};
    KeyCode code;
    XUngrabKey(display, AnyKey, AnyModifier, Root);
    for (k = 0; k < LENGTH(keys); k++) {
        if ((code = XKeysymToKeycode(display, keys[k].keysym))) {
            for (j = 0; j < LENGTH(modifiers); j++) {
                XGrabKey(display, code, keys[k].mod | modifiers[j], Root, True,
                         GrabModeAsync, GrabModeAsync);
            }
        }
    }
}

struct client *getNextWindow(Window window, int Index) {
    if (window == None || window == Root) {
        return monitors[Index].clients;
    }
    for (struct client *w = monitors[Index].clients; w && w->last->window;
         w = w->last) {
        if (w->window == window) {
            return w->last;
        }
    }
    return monitors[Index].clients;
}

void SwitchClients() {
    struct client *tmp =
            getNextWindow(GET_CURRENT_WINDOW(MonitorIndex), MonitorIndex);
    if (tmp && windowExist(tmp->window, MonitorIndex) && tmp->window != Root &&
        tmp->window != panel.window) {
        upWindow(tmp->window);
    }
}

void upWindow(Window window) {
    if (WINDOW_IS_NOT_VALID)
        return;
    XWindowChanges wa;
    XSetWindowAttributes xswa;

    struct client *windows = getWindow(window, MonitorIndex);
    if (windows == NULL)
        return;

    XMapWindow(display, window);
    wa.x = windows->x;
    wa.y = windows->y;
    wa.width = windows->width;
    wa.height = windows->height;
    XConfigureWindow(display, window, CWX | CWY | CWWidth | CWHeight, &wa);
    XRaiseWindow(display, window);
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
    Atom atom_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    setWindowProperty(display, window, "_NET_WM_STATE", atom_above);
    updateCurrentWindow(window, MonitorIndex);
    xswa.event_mask = WM_ATRIB;
    XChangeWindowAttributes(display, Root, CWEventMask, &xswa);
    XChangeWindowAttributes(display, window, CWEventMask, &xswa);
}

bool windowExist(Window window, int Index) {
    if (WINDOW_IS_NOT_VALID) {
        return false;
    }
    XWindowAttributes attrs;
    return XGetWindowAttributes(display, window, &attrs);
}

struct client *getWindow(Window window, int Index) {
    if (window == Root || window == 0) {
        return NULL;
    }
    for (struct client *tmp = monitors[Index].clients; tmp && tmp->window;
         tmp = tmp->last) {
        if (tmp->window == window) {
            return tmp;
        }
    }
    return NULL;
}

void FullScreen() {
    Window current = GET_CURRENT_WINDOW(MonitorIndex);
    if (current == panel.window)
        return;
    info tmp = query(current);

    int max_width = DisplayWidth(display, tmp.screenNumber);
    int max_height = DisplayHeight(display, tmp.screenNumber);
    int x = 0, y = 0;

    getResolution(display, &max_width, &max_height, &x, &y);


    struct client *windows = getNextWindow(current, MonitorIndex);

    if (windows == NULL)
        return;

    XMoveResizeWindow(display, current, x, y, max_width, max_height);

    Atom atom_fullscreen =
            XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (atom_fullscreen != None) {
        XChangeProperty(display, current,
                        XInternAtom(display, "_NET_WM_STATE", False), XA_ATOM, 32,
                        PropModeReplace, (unsigned char *) &atom_fullscreen, 1);
    }

    windows->width = max_width;
    windows->height = max_height;
    windows->x = x;
    windows->y = y;

    XSync(display, False);
}

info query(Window w) {
    Window rootWindow;
    Window childWindow;
    int rootX, rootY, winX, winY;
    unsigned int mask;
    int screenNumber = 0;

    screenNumber = DefaultScreen(display);
    XWindowAttributes windowAttrs;
    if (XGetWindowAttributes(display, w, &windowAttrs)) {
        for (int i = 0; i < ScreenCount(display); i++) {
            if (windowAttrs.screen == ScreenOfDisplay(display, i)) {
                screenNumber = i;
                break;
            }
        }
    }

    // Use XQueryPointer to get the root window coordinates of the window
    if (XQueryPointer(display, w, &rootWindow, &childWindow, &rootX, &rootY,
                      &winX, &winY, &mask)) {

    } else {
        rootWindow = childWindow = rootX = rootY = winX = winY = mask = None;
    }
    info tmp = {.rootWindow = rootWindow,
                .childWindow = childWindow,
                .rootX = rootX,
                .rootY = rootY,
                .winX = winX,
                .winY = winY,
                .mask = mask,
                .screenNumber = screenNumber};
    return tmp;
}

void Maximize() {
    Window current = GET_CURRENT_WINDOW(MonitorIndex);
    if (current == panel.window)
        return;

    info tmp = query(current);

    int max_width = DisplayWidth(display, tmp.screenNumber);
    int max_height = DisplayHeight(display, tmp.screenNumber) - panel.height;

    int x = 0, y = 0;

    getResolution(display, &max_width, &max_height, &x, &y);
    y = panel.height;

    XMoveResizeWindow(display, current, x, y, max_width, max_height);
    Atom atom_fullscreen =
            XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    if (atom_fullscreen != None) {
        XChangeProperty(display, current,
                        XInternAtom(display, "_NET_WM_STATE", False), XA_ATOM, 32,
                        PropModeReplace, (unsigned char *) &atom_fullscreen, 1);
    }

    struct client *windows = getWindow(current, MonitorIndex);
    if (windows == NULL)
        return;

    windows->width = max_width;
    windows->height = max_height;
    windows->x = x;
    windows->y = y;

    XSync(display, False);
}


void resize(const Arg *arg) {
    XEvent e;
    XMotionEvent *ev = &e.xmotion;
    int dx;
    int dy;

    Window current = GET_CURRENT_WINDOW(MonitorIndex);

    if (XGrabPointer(display, Root, False, MOUSE_MASK, GrabModeAsync,
                     GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
        return;
    }

    dx = 1;
    dy = 1;

    struct client *windows = getWindow(current, MonitorIndex);
    if (windows == NULL)
        return;
    XSizeHints *hints;
    long suppliedHints;
    hints = XAllocSizeHints();
    XGetWMNormalHints(display, current, hints, &suppliedHints);


    do {
        // if ((suppliedHints & PMinSize) && (windows->width < hints->min_width || windows->height < hints->min_height)) {
        //     ev->x = MAX(ev->x - dx,windows->width);
        //     ev->y = MAX(ev->y - dy,windows->height);
        // }else if ((suppliedHints & PMaxSize) && (windows->width > hints->max_width || windows->height > hints->max_height)) {
        //     ev->x = MAX(ev->x - dx,windows->width);
        //     ev->y = MAX(ev->y - dy,windows->height);
        // }else{
        //     ev->x = ev->x - dx;
        //     ev->y = ev->y - dy;
        // }
        XMaskEvent(display, MOUSE_MASK | ExposureMask | SubstructureRedirectMask,
                   &e);
        XResizeWindow(display, monitors[MonitorIndex].current->window, ev->x,
                      ev->y);
        windows->width = ev->x;
        windows->height = ev->y;
        monitors[MonitorIndex].current->width = ev->x;
        monitors[MonitorIndex].current->height = ev->y;
        XSync(display, False);
    } while (ev->type != ButtonRelease);
    XMaskEvent(display, ButtonReleaseMask, &e);
    XUngrabPointer(display, CurrentTime);
    return;
}


void move(const Arg *arg) {
    XEvent e;

    Window current = GET_CURRENT_WINDOW(MonitorIndex);
    struct client *windows = getWindow(current, MonitorIndex);
    if (windows == NULL)
        return;
    XMotionEvent *ev = &e.xmotion;
    if (XGrabPointer(display, Root, False, MOUSE_MASK, GrabModeAsync,
                     GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
        return;
    }
    do {
        XMaskEvent(display, MOUSE_MASK | ExposureMask | SubstructureRedirectMask,
                   &e);
        XMoveWindow(display, monitors[MonitorIndex].current->window, ev->x, ev->y);
        windows->x = ev->x;
        windows->y = ev->y;
    } while (ev->type != ButtonPress);
    XUngrabPointer(display, CurrentTime);
}

void focus(const Arg *arg) {
    Window current = GET_CURRENT_WINDOW(MonitorIndex);
    printf("[%lu] focus request\n", current);
}
/**
 * -------[.......screen.......]
 * mode 0 window|rest of screen
 * mode 1 rest of screen|window
 * mode 2 re arrange all screen alongside horizontally
 * mode 3 re arrange all screen alongside vertically
 * mode 4 re arrange all screen tiling mode
 */
void Arrange(const Arg *arg) {
    Window current = GET_CURRENT_WINDOW(MonitorIndex);
    if (current == Root || current == None) {
        return;
    }
    int width = DisplayWidth(display, DefaultScreen(display));
    int height = DisplayHeight(display, DefaultScreen(display)) - panel.height;


    int x = 0, y = 0;

    getResolution(display, &width, &height, &x, &y);
    if (arg->i == 0) {
        monitors[MonitorIndex].current->width = width / 2;
        monitors[MonitorIndex].current->height = height;
        monitors[MonitorIndex].current->x = x;
        monitors[MonitorIndex].current->y = panel.height;
    }

    if (arg->i == 1) {
        monitors[MonitorIndex].current->width = width / 2;
        monitors[MonitorIndex].current->height = height;
        monitors[MonitorIndex].current->x = x + width / 2;
        monitors[MonitorIndex].current->y = panel.height;
    }

    upWindow(current);
}

void SwitchMonitor(const Arg *arg) {
    if (arg->i == MonitorIndex || arg->i < 0 || arg->i > MAX_MONITOR) {
        return;
    }
    setWorkspaceNumber(display, Root, (unsigned long) arg->i);
    struct Monitor *d = &monitors[MonitorIndex];
    struct Monitor *n = &monitors[arg->i];

    // Disable SubstructureNotify events to prevent them from being propagated
    XSetWindowAttributes attr = {.do_not_propagate_mask = SubstructureNotifyMask};
    XChangeWindowAttributes(display, Root, CWEventMask, &attr);
    // Unmap all clients in the old workspace
    for (struct client *c = d->clients; c && c->window; c = c->last) {
        XUnmapWindow(display, c->window);
        if (isStickWindows(c->window) || isPanel(c->window)) {
            modifyMonitor(c->window, MonitorIndex, arg->i);
        }
    }

    // map panel
    if (panel.window != None) {
        XMapWindow(display, panel.window);
        upWindow(panel.window);
    }
    // Map all clients in the new workspace and bring theme to the top
    for (struct client *c = n->clients;
         c && c->window && c->window != Root && c->window != panel.window;
         c = c->last) {
        XWindowChanges wa;
        XMapWindow(display, c->window);
        wa.x = c->x;
        wa.y = c->y;
        wa.width = c->width;
        wa.height = c->height;
        XConfigureWindow(display, c->window, CWX | CWY | CWWidth | CWHeight, &wa);
        XRaiseWindow(display, c->window);
        XSetInputFocus(display, c->window, RevertToPointerRoot, CurrentTime);
        updateCurrentWindow(c->window, MonitorIndex);
        upWindow(c->window);
    }

    MonitorIndex = arg->i;
    // Enable the necessary events again
    attr.event_mask = WM_ATRIB;
    XChangeWindowAttributes(display, Root, CWEventMask, &attr);

    // Bring the current client to the top
    struct client *windows =
            getWindow(GET_CURRENT_WINDOW(MonitorIndex), MonitorIndex);
    if (windows != NULL) {
        upWindow(windows->window);
    }
    XSync(display, False);
}

void MoveToMonitor(const Arg *arg) {
    if (arg->i == MonitorIndex || arg->i < 0 || arg->i > MAX_MONITOR) {
        return;
    }
    // if the window is sticky . dont do that.
    Window window = GET_CURRENT_WINDOW(MonitorIndex);
    if (WINDOW_IS_NOT_VALID) {
        return;
    }
    modifyMonitor(window, MonitorIndex, arg->i);
    XUnmapWindow(display, window);
    XSync(display, False);
}

void sendCloseEvent(Window window) {
    Atom WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", False);
    Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

    XEvent ev;
    ev.xclient.type = ClientMessage;
    ev.xclient.window = window;
    ev.xclient.message_type = WM_PROTOCOLS;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = WM_DELETE_WINDOW;

    XSendEvent(display, window, False, NoEventMask, &ev);
    XFlush(display);
}

void KillWindow() {
    Window window = GET_CURRENT_WINDOW(MonitorIndex);
    printf("killing window %lu\n", window);
    if (WINDOW_IS_NOT_VALID) {
        return;
    }
    Atom *supported_protocols;
    int num_supported_protocols;
    if (XGetWMProtocols(display, window, &supported_protocols,
                        &num_supported_protocols)) {
        sendCloseEvent(window);
        return;
    }

    XGrabServer(display);
    struct client *tmp = getNextWindow(window, MonitorIndex);
    XKillClient(display, window);
    // Unmap the client window first
    XUnmapWindow(display, window);
    if (tmp->window != None) {
        XReparentWindow(display, window, tmp->window, 0, 0);
    } else {
        XReparentWindow(display, window, Root, 0, 0);
    }
    // Remove the client window from the save set
    XRemoveFromSaveSet(display, window);

    // Remove the client from the current workspace and free its memory
    removeWindow(window, MonitorIndex);

    // Synchronize with the X server to ensure all changes take effect
    XSync(display, False);

    // Destroy the frame window
    if (window) {
        XDestroyWindow(display, window);
    }
    XUngrabServer(display);
}

void run() {
    while (True) {
        XEvent e;
        XNextEvent(display, &e);
        if (events[e.type]) {
            events[e.type](&e);
        }
    }
}

void buttonpress(XEvent *e) {
    XButtonEvent *ev = &e->xbutton;
    Arg arg = {0};
    int click = ClkClientWin;
    for (int i = 0; i < LENGTH(buttons); i++) {
        if (click == buttons[i].click && buttons[i].func &&
            buttons[i].button == ev->button &&
            CLEAN_MASK(buttons[i].mask) == CLEAN_MASK(ev->state)) {
            buttons[i].func(
                    click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
        }
        XUngrabButton(display, AnyButton, AnyModifier, ev->window);
    }
}

void OnMapingNotify(XEvent *e) {
    XSync(display, false);
}
void Onkey(XEvent *e) {
    unsigned int j = 0;
    XKeyEvent *ev;
    ev = &e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(display, e->xkey.keycode, 0, 0);
    for (j = 0; j < LENGTH(keys); j++) {
        if (keysym == keys[j].keysym &&
            CLEAN_MASK(keys[j].mod) == CLEAN_MASK(ev->state) && keys[j].func) {
            keys[j].func(&keys[j].arg);
        }
    }
}

void justprint(XEvent *e) { printf("[+] Info : event type %d\n", e->type); }

void onConfigureNotify(XEvent *e) {
    XFlush(display);
}

void onConfigureRequest(XEvent *e) {
    XConfigureRequestEvent *configureRequest = &(e->xconfigurerequest);

    printf("on configureRequest- %lu\n", configureRequest->window);
    if (configureRequest->window == Root || configureRequest->window == None) {
        return;
    }
    XWindowChanges changes;

    changes.x = configureRequest->x;
    changes.y = configureRequest->y;
    changes.width = configureRequest->width;
    changes.height = configureRequest->height;

    XConfigureWindow(display, configureRequest->window,
                     CWX | CWY | CWWidth | CWHeight, &changes);
}

void onMotionNotify(XEvent *e) {
    XMotionEvent *motionEvent = &(e->xmotion);
    printf("Mouse moved to position: %d,%d\n", motionEvent->x, motionEvent->y);

    if (motionEvent->window == Root || motionEvent->window == None) {
        return;
    }

    // Perform actions based on the mouse movement
    if (motionEvent->x < 100 && motionEvent->y < 100) {
        printf("Mouse is in the top-left corner\n");
    } else if (motionEvent->x > 300 && motionEvent->y > 200) {
        printf("Mouse is in the bottom-right corner\n");
    }

    // Change the window title based on the mouse position
    char title[50];
    sprintf(title, "Mouse position: %d,%d", motionEvent->x, motionEvent->y);
    XStoreName(display, motionEvent->window, title);

    // Update the window to reflect the title change
    XFlush(display);
}

void setFocus(XEvent *e) { XFocusChangeEvent *ev = &e->xfocus; }

void OnUnmapNotify(XEvent *e) {
    XUnmapEvent *ev = &e->xunmap;
    if (ev->window == Root || ev->window == None) {
        return;
    }
    bool exist = getWindow(ev->window, MonitorIndex) == NULL;
    if (!exist) {
        Unframe(ev);
    }
    XSync(display, False);
}

void OnExpose(XEvent *e) {
    XFlush(display);
}

void printAtomName(Display *display, Atom atom, Window win) {
    char *atomName = XGetAtomName(display, atom);

    if (atomName != NULL) {
        printf("[%lu] Atom Name: %s\n", win, atomName);
        XFree(atomName);
    } else {
        printf("Failed to get atom name.\n");
    }
}

Bool doesWindowExist(Window window) {
    XWindowAttributes windowAttrs;
    return XGetWindowAttributes(display, window, &windowAttrs);
}

long getAtomValue(Window window, Atom atom) {
    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char *propData;

    if (XGetWindowProperty(display, window, atom, 0, 1, False, AnyPropertyType,
                           &actualType, &actualFormat, &nItems, &bytesAfter, &propData) == Success) {
        if (actualType != None) {
            long atomValue = *(long *) propData;
            XFree(propData);
            return atomValue;
        }
        XFree(propData);
    }
    return 0;
}

void OnPropertyNotify(XEvent *e) {
    EventMapping eventMap[] = {
            {XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False), updatePanelInfo},
            {XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False), updateDesktop},
            {XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False), updatePanelInfo},
    };

    XPropertyEvent *ev = &e->xproperty;
    size_t numEvents = sizeof(eventMap) / sizeof(EventMapping);
    Atom atom = ev->atom;
    printAtomName(display, atom, ev->window);
    for (size_t i = 0; i < numEvents; ++i) {
        if (eventMap[i].atom == getAtomValue(ev->window, atom)) {
            eventMap[i].handler(e);
            break;
        }
    }
}

void Unframe(XUnmapEvent *ev) {
    XSync(display, False);
    struct client *tmp = getNextWindow(ev->window, MonitorIndex);
    if (tmp == NULL)
        return;
    if (tmp->window) {
        updateCurrentWindow(tmp->window, MonitorIndex);
    } else {
        updateCurrentWindow(-1, MonitorIndex);
    }
    removeWindow(ev->window, MonitorIndex);
}

void OnMapRequest(XEvent *e) {
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;
    if (ev->window == Root || ev->window == None) {
        return;
    }
    if (!XGetWindowAttributes(display, ev->window, &wa)) {
        return;
    }
    if (wa.override_redirect) {
        return;
    }
    Frame(ev->window);
}

void OnmapNotify(XEvent *e) {
    XMapEvent *mapEvent = &(e->xmap);

    Window mappedWindow = mapEvent->window;

    if (mappedWindow == Root || mappedWindow == None) {
        return;
    }
    printf("Window mapped: %lu\n", mappedWindow);

    XStoreName(display, mappedWindow, "Mapped Window");

    XClearWindow(display, mappedWindow);
}

void OnDestroyNotify(XEvent *event) {
    XDestroyWindowEvent *destroyEvent = &event->xdestroywindow;
    if (destroyEvent->window == Root ||
        !windowExist(destroyEvent->window, MonitorIndex)) {
        return;
    }


    XReparentWindow(display, destroyEvent->window, Root, 0, 0);

    XMapWindow(display, destroyEvent->window);
}

void OnEnterNotify(XEvent *e) {
    XEnterWindowEvent *ev = &e->xcrossing;
    //upWindow(ev->window);
}

void Frame(Window window) {
    if (WINDOW_IS_NOT_VALID) {
        return;
    }
    XSizeHints sizeHints;

    XWindowAttributes x_window_attrs;
    XGetWindowAttributes(display, window, &x_window_attrs);
    XSetWindowAttributes wa;
    int x, y, z, v, width, height;
    x = y = 0;
    unsigned int mask;
    long suppliedHints;
    Window root_return, child_return;

    XSelectInput(display, window,
                 EnterWindowMask | PointerMotionMask | FocusChangeMask |
                         PropertyChangeMask | StructureNotifyMask);
    XAddToSaveSet(display, window);
    XMapWindow(display, window);
    addWindow(window, MonitorIndex);
    width = x_window_attrs.width;
    height = x_window_attrs.height;


    if (XGetWMNormalHints(display, window, &sizeHints, &suppliedHints) != 0) {
        if (sizeHints.flags & PPosition) {
            x = sizeHints.x;
            y = sizeHints.y;
        }

        if (sizeHints.flags & PSize) {
            width = MIN(M_WIDTH, sizeHints.width);
            height = MIN(M_HEIGHT, sizeHints.height);
        }

        if (sizeHints.flags & PMinSize) {
            width = MAX(M_WIDTH, sizeHints.min_width);
            height = MAX(M_HEIGHT, sizeHints.min_height);
        }
    }


    int max_width, max_height, xx, yy;
    getResolution(display, &max_width, &max_height, &xx, &yy);
    if (isPanel(window)) {
        monitors[MonitorIndex].clients->x = 0;
        monitors[MonitorIndex].clients->y = 0;
    } else if (x || y) {
        monitors[MonitorIndex].clients->x = x;
        monitors[MonitorIndex].clients->y = y;
    } else if (!x && !y) {
        if (
                monitors[MonitorIndex].x_init > max_width / 2) {
            monitors[MonitorIndex].x_init = max_width / 2;
        }
        if (
                monitors[MonitorIndex].y_init > max_height / 2) {
            monitors[MonitorIndex].y_init = max_height / 2;
        }
        monitors[MonitorIndex].clients->x = monitors[MonitorIndex].x_init;
        monitors[MonitorIndex].clients->y = monitors[MonitorIndex].y_init;
        monitors[MonitorIndex].x_init += 1;
        monitors[MonitorIndex].y_init += 5;
    } else {
        XQueryPointer(display, window, &root_return, &child_return, &z, &v, &x, &y, &mask);
        monitors[MonitorIndex].clients->x = x;
        monitors[MonitorIndex].clients->y = y;
    }

    monitors[MonitorIndex].clients->width = width;
    monitors[MonitorIndex].clients->height = height;

    upWindow(window);
    wa.event_mask = WM_ATRIB;
    XSelectInput(display, Root, wa.event_mask);

    updateCurrentWindow(window, MonitorIndex);
    XRaiseWindow(display, window);
    XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
    // Set the _NET_WM_ICON property
    unsigned char iconData[] = {
            // Icon pixel data
            0x00, 0x00, 0x00, 0x00,// Example pixel values
            0xFF, 0xFF, 0xFF, 0xFF,// Example pixel values
                                   // ... more pixel values
    };

    XChangeProperty(display, window, XInternAtom(display, "_NET_WM_ICON", False),
                    XA_CARDINAL, 32, PropModeReplace, iconData, sizeof(iconData));

    setMotifWMHints(display, window, motifHints,
                    sizeof(motifHints) / sizeof(motifHints[0]));

    Atom atom_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    setWindowProperty(display, window, "_NET_WM_WINDOW_TYPE", atom_type);
}

void setMotifWMHints(Display *display, Window window, unsigned long *hints,
                     int numHints) {
    Atom motifHintsAtom = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    XChangeProperty(display, window, motifHintsAtom, motifHintsAtom, 32,
                    PropModeReplace, (unsigned char *) hints, numHints);
}

void setWindowProperty(Display *display, Window window,
                       const char *propertyName, Atom propertyValue) {
    Atom atom_property = XInternAtom(display, propertyName, False);
    XChangeProperty(display, window, atom_property, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *) &propertyValue, 1);
    XSync(display, False);
}

void getWindowProperty(Display *display, Window window,
                       const char *propertyName, char *propertyType,
                       void **propertyValue, unsigned long *numItems) {
    Atom atom_property = XInternAtom(display, propertyName, False);
    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char *propValue = NULL;

    int status = XGetWindowProperty(display, window, atom_property, 0, 1, False,
                                    AnyPropertyType, &actualType, &actualFormat,
                                    &nItems, &bytesAfter, &propValue);
    if (status == Success && actualFormat == 32) {
        *propertyValue = malloc(sizeof(Atom));
        memcpy(*propertyValue, propValue, sizeof(Atom));
        *numItems = 1;
    }

    XFree(propValue);
}

bool getWindow_NET_WM_Atom(Window window, char *AtomName) {
    unsigned long num_items;
    unsigned char *prop_value = NULL;

    getWindowProperty(display, window, "_NET_WM_STATE", "ATOM",
                      (void **) &prop_value, &num_items);

    if (prop_value != NULL) {
        Atom *atoms = (Atom *) prop_value;
        for (unsigned long i = 0; i < num_items; i++) {
            if (atoms[i] == XInternAtom(display, AtomName, False)) {
                XFree(prop_value);
                return true;
            }
        }
        XFree(prop_value);
    }
    return false;
}

bool getWindow_EWMH_Atom(Window window, char *AtomName) {
    unsigned long num_items;
    unsigned char *prop_value = NULL;

    getWindowProperty(display, window, "_NET_WM_WINDOW_TYPE", "ATOM",
                      (void **) &prop_value, &num_items);
    if (prop_value != NULL) {
        Atom *atoms = (Atom *) prop_value;
        for (unsigned long i = 0; i < num_items; i++) {
            if (atoms[i] == XInternAtom(display, AtomName, False)) {
                XFree(prop_value);
                return true;
            }
        }
        XFree(prop_value);
    }
    return false;
}

bool isStickWindows(Window window) {
    return getWindow_NET_WM_Atom(window, "_NET_WM_STATE_STICKY");
}

bool isAboveWindows(Window window) {
    return getWindow_NET_WM_Atom(window, "_NET_WM_STATE_ABOVE");
}

bool isPanel(Window window) {
    return getWindow_EWMH_Atom(window, "_NET_WM_WINDOW_TYPE_DOCK");
}

void updatePanelInfo(XEvent *e) {
    XPropertyEvent *ep = &e->xproperty;
    if (isPanel(ep->window)) {
        panel.window = ep->window;
        XWindowAttributes x_window_attrs;
        XGetWindowAttributes(display, ep->window, &x_window_attrs);
        panel.height = x_window_attrs.height;
        panel.width = x_window_attrs.width;
        panel.position = 0;
    }
}
void updateDesktop(XEvent *e) {

    printf("exit on updateDesktop request");
    exit(1);
}


int main() {
    dummyClient();
    initRootWidow();
    // XSynchronize(display, True);
    run();
    return 0;
}
