#include "Momentum.h"
#include "X11/X.h"
#include "X11/Xlib.h"
#include "X11/Xproto.h"
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <string.h>

struct Monitor monitors[MAXMONITOR];

//_NET_WM_STATE_MODAL
//_NET_WM_STATE_STICKY
//_NET_WM_STATE_MAXIMIZED_VERT
//_NET_WM_STATE_MAXIMIZED_HORZ
//_NET_WM_STATE_SHADED
//_NET_WM_STATE_SKIP_TASKBAR
//_NET_WM_STATE_SKIP_PAGER
//_NET_WM_STATE_HIDDEN
//_NET_WM_STATE_FULLSCREEN
//_NET_WM_STATE_ABOVE
//_NET_WM_STATE_BELOW
//_NET_WM_STATE_DEMANDS_ATTENTION
// EWMH
// https://specifications.freedesktop.org/wm-spec/1.1/x104.html

bool addWindow(Window window, int Index) {
  if (window == Root || window == 0)
    return false;
  if (Index < 0 || Index > MAXMONITOR)
    return false;
  struct Windows *windows = malloc(sizeof(struct Windows));
  windows->last = monitors[Index].windows;
  windows->window = window;
  windows->identefire = -1;
  monitors[Index].windows = windows;
  updateCurrentWindow(window, Index);
  return true;
}

bool removeWindow(Window window, int Index) {
  if (window == Root || window == None) {
    return false;
  }
  if (Index < 0 || Index > MAXMONITOR)
    return false;

  if (window == panel.window) {
    panel.window = None;
    panel.width = panel.height = panel.position = 0;
  }

  for (struct Windows **windows = &monitors[Index].windows;
       windows && (*windows)->window; windows = &(*windows)->last) {
    if ((*windows)->window == window) {
      *windows = (*windows)->last;
      updateCurrentWindow(-1, Index);
      return true;
    }
  }
  return false;
}

void dummyClient(void) {
  for (int i = 0; i <= MAXMONITOR; i++) {
    monitors[i].windows = malloc(sizeof(struct Windows));
    monitors[i].windows->last = malloc(sizeof(struct Windows));
    monitors[i].current = monitors[i].windows;
  }
  panel.window = None;
  panel.height = panel.width = 0;
}

void getWindowsData() {
  for (int i = 0; i <= MAXMONITOR; i++) {
    Window current = monitors[i].current != (struct Windows *)NULL
                         ? monitors[i].current->window
                         : (Window)-1;
    if (current == None)
      return;
    printf("%d : { current : %lu,  windows :", i, current);
    if (monitors[i].current->window != (Window)NULL) {
      for (struct Windows *windows = monitors[i].windows;
           windows && windows->window && windows->last;
           windows = windows->last) {
        printf("   %lu : %d", windows->window, windows->identefire);
      }
    }
    printf("  },\n");
  }
}

bool updateCurrentWindow(Window window, int Index) {
  if (window == Root || window == 0)
    return false;
  if (Index < 0 || Index > MAXMONITOR)
    return false;

  for (struct Windows *windows = monitors[Index].windows;
       windows && windows->window; windows = windows->last) {
    if (windows->window == window) {
      monitors[Index].current = windows;
      return true;
    }
  }
  monitors[Index].current = monitors[Index].windows;
  return false;
}

bool updateWindow(struct Windows *windows, int Index) {
  if (windows->window == Root || windows->window == 0)
    return false;
  if (Index < 0 || Index > MAXMONITOR)
    return false;
  for (struct Windows **tmp = &monitors[Index].windows; tmp && (*tmp)->window;
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
                    PropModeReplace, (unsigned char *)&workspace_number, 1);
    XSync(display, False);
  } else {
    printf("Failed to get workspace atom\n");
  }
}

bool modifyMonitor(Window window, int Index, int NextIndex) {
  if (window == Root || window == 0)
    return false;
  if (Index < 0 || Index > MAXMONITOR || NextIndex < 0 ||
      NextIndex > MAXMONITOR)
    return false;

  struct Windows **tmp;

  for (tmp = &monitors[Index].windows; tmp && (*tmp)->window && (*tmp)->last;
       tmp = &(*tmp)->last) {
    if (window == (*tmp)->window) {
      monitors[NextIndex].windows->last = monitors[NextIndex].windows;
      monitors[NextIndex].windows = *tmp;
      *tmp = (*tmp)->last;
      if (monitors[Index].current->window == window) {
        updateCurrentWindow(-1, Index);
        return true;
      }
      updateCurrentWindow(window, NextIndex);
      return true;
    }
  }
  return false;
}

void execute(const Arg *arg) {
  if (fork() == 0) {
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    fprintf(stderr, "Momentum: Execute %s", ((char **)arg->v)[0]);
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
  if (XStringListToTextProperty((char **)&wmNameString, 1, &wmName) == 0) {
    fprintf(stderr, "Failed to set window manager name.\n");
    return 1;
  }

  Screen *screen = XScreenOfDisplay(display, 0);
  Root = XRootWindowOfScreen(screen);
  XSetWMName(display, Root, &wmName);

  XSetErrorHandler(OnXError);
  XGrabServer(display);
  XSync(display, False);
  XSelectInput(display, Root,
               SubstructureRedirectMask | SubstructureNotifyMask);
  XUngrabServer(display);
  grabkeys();
  setWorkspaceNumber(display, Root, (unsigned long)0);
  MonitorIndex = 1;
  return True;
}

int OnXError(Display *display, XErrorEvent *event) {
  char error_text[MAXLENGHT];
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

struct Windows *getNextWindow(Window window, int Index) {
  if (window == None || window == Root) {
    return monitors[Index].windows;
  }
  for (struct Windows *w = monitors[Index].windows; w && w->last->window;
       w = w->last) {
    if (w->window == window) {
      return w->last;
    }
  }
  return monitors[Index].windows;
}

void SwitchWindows() {
  struct Windows *tmp =
      getNextWindow(GETCURRENTWINDOW(MonitorIndex), MonitorIndex);
  if (tmp && windowExist(tmp->window, MonitorIndex) && tmp->window != Root &&
      tmp->window != panel.window) {
    upWindow(tmp->window);
  }
}

void upWindow(Window window) {
  if (window == Root || window == None)
    return;
  XWindowChanges wa;

  struct Windows *windows = getWindow(window, MonitorIndex);
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
}

bool windowExist(Window window, int Index) {
  if (window == Root || window == 0) {
    return false;
  }
  XWindowAttributes attrs;
  return XGetWindowAttributes(display, window, &attrs);
}

struct Windows *getWindow(Window window, int Index) {
  if (window == Root || window == 0) {
    return NULL;
  }
  for (struct Windows *tmp = monitors[Index].windows; tmp && tmp->window;
       tmp = tmp->last) {
    if (tmp->window == window) {
      return tmp;
    }
  }
  return NULL;
}

void FullScreen() {
  int current = GETCURRENTWINDOW(MonitorIndex);
  if (current == panel.window)
    return;
  int screen = DefaultScreen(display);
  int max_width = DisplayWidth(display, screen);
  int max_height = DisplayHeight(display, screen);

  struct Windows *windows = getNextWindow(current, MonitorIndex);
  if (windows == NULL)
    return;

  XMoveResizeWindow(display, current, 0, 0, max_width, max_height);

  Atom atom_fullscreen =
      XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
  if (atom_fullscreen != None) {
    XChangeProperty(display, current,
                    XInternAtom(display, "_NET_WM_STATE", False), XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&atom_fullscreen, 1);
  }

  windows->width = max_width;
  windows->height = max_height;
  windows->x = 0;
  windows->y = 0;

  XSync(display, False);
}

void Maximize() {
  int current = GETCURRENTWINDOW(MonitorIndex);
  if (current == panel.window)
    return;
  int screen = DefaultScreen(display);
  int max_width = DisplayWidth(display, screen);
  int max_height = DisplayHeight(display, screen) - panel.height;
  int x = 0;
  int y = panel.height;
  XMoveResizeWindow(display, current, x, y, max_width, max_height);
  Atom atom_fullscreen =
      XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  if (atom_fullscreen != None) {
    XChangeProperty(display, current,
                    XInternAtom(display, "_NET_WM_STATE", False), XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&atom_fullscreen, 1);
  }

  struct Windows *windows = getWindow(current, MonitorIndex);
  if (windows == NULL)
    return;

  windows->width = max_width;
  windows->height = max_height;
  windows->x = x;
  windows->y = y;

  XSync(display, False);
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
  Window current = GETCURRENTWINDOW(MonitorIndex);
  if (current == Root || current == None) {
    return;
  }
  int screen = DefaultScreen(display);
  int width = DisplayWidth(display, screen);
  int height = DisplayHeight(display, screen) - panel.height;

  if (arg->i == 0) {
    monitors[MonitorIndex].current->width = width/2;
    monitors[MonitorIndex].current->height = height;
    monitors[MonitorIndex].current->x = 0;
    monitors[MonitorIndex].current->y = panel.height;
  }

  if (arg->i == 1) {
    monitors[MonitorIndex].current->width = width/2;
    monitors[MonitorIndex].current->height = height;
    monitors[MonitorIndex].current->x = width/2;
    monitors[MonitorIndex].current->y = panel.height;
  }

  upWindow(current);

}
void SwitchMonitor(const Arg *arg) {
  if (arg->i == MonitorIndex || arg->i < 0 || arg->i > MAXMONITOR) {
    return;
  }
  setWorkspaceNumber(display, Root, (unsigned long)arg->i);
  struct Monitor *d = &monitors[MonitorIndex];
  struct Monitor *n = &monitors[arg->i];

  // Disable SubstructureNotify events to prevent them from being propagated
  XSetWindowAttributes attr = {.do_not_propagate_mask = SubstructureNotifyMask};
  XChangeWindowAttributes(display, Root, CWEventMask, &attr);
  // Unmap all clients in the old workspace
  for (struct Windows *c = d->windows; c && c->window; c = c->last) {
    XUnmapWindow(display, c->window);
    if (isStickWindows(c->window) || isPanel(c->window)) {
      bool status = modifyMonitor(c->window, MonitorIndex, arg->i);
    }
  }

  // map panel
  if (panel.window != None) {
    XMapWindow(display, panel.window);
    upWindow(panel.window);
  }
  // Map all clients in the new workspace and bring theme to the top
  for (struct Windows *c = n->windows;
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
  attr.event_mask = SubstructureRedirectMask | ButtonPressMask |
                    SubstructureNotifyMask | PropertyChangeMask;
  XChangeWindowAttributes(display, Root, CWEventMask, &attr);

  // Bring the current client to the top
  struct Windows *windows =
      getWindow(GETCURRENTWINDOW(MonitorIndex), MonitorIndex);
  if (windows != NULL) {
    upWindow(windows->window);
  }
  XSync(display, False);
}

void MoveToMonitor(const Arg *arg) {
  if (arg->i == MonitorIndex || arg->i < 0 || arg->i > MAXMONITOR) {
    return;
  }
  // if the window is sticky . dont do that.
  Window window = GETCURRENTWINDOW(MonitorIndex);
  if (window == Root || window == None) {
    return;
  }
  modifyMonitor(window, MonitorIndex, arg->i);
  XUnmapWindow(display, window);
  XSync(display, False);
}
void KillWindow() {
  Window w = GETCURRENTWINDOW(MonitorIndex);

  if (w == Root || w == None) {
    return;
  }
  XGrabServer(display);

  XKillClient(display, w);
  // Unmap the client window first
  XUnmapWindow(display, w);

  // Reparent the client window back to the root window
  XReparentWindow(display, w, Root, 0, 0);

  // Remove the client window from the save set
  XRemoveFromSaveSet(display, w);

  // Remove the client from the current workspace and free its memory
  removeWindow(w, MonitorIndex);

  // Synchronize with the X server to ensure all changes take effect
  XSync(display, False);

  // Destroy the frame window
  if (w) {
    XDestroyWindow(display, w);
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

void Onkey(XEvent *e) {
  unsigned int j = 0;
  XKeyEvent *ev;
  ev = &e->xkey;
  KeySym keysym = XkbKeycodeToKeysym(display, e->xkey.keycode, 0, 0);
  for (j = 0; j < LENGTH(keys); j++) {
    if (keysym == keys[j].keysym &&
        CLEANMASK(keys[j].mod) == CLEANMASK(ev->state) && keys[j].func) {
      if (keys[j].func) {
        keys[j].func(&keys[j].arg);
      }
    }
  }
}
void justprint(XEvent *e) { printf("[+] Info : event type %d\n", e->type); }

void onConfigureNotify(XEvent *e) {
  XConfigureEvent *configureEvent = &(e->xconfigure);

  if (configureEvent->window == Root || configureEvent->window == None) {
    return;
  }
  printf(
      "Window %lu has been configured. New position: %d,%d, new size: %d,%d\n",
      configureEvent->window, configureEvent->x, configureEvent->y,
      configureEvent->width, configureEvent->height);
  XWindowChanges changes;
  changes.x = configureEvent->x;
  changes.y = configureEvent->y;
  changes.width = configureEvent->width;
  changes.height = configureEvent->height;

  XConfigureWindow(display, configureEvent->window,
                   CWX | CWY | CWWidth | CWHeight, &changes);
}

void onConfigureRequest(XEvent *e) {
  XConfigureRequestEvent *configureRequest = &(e->xconfigurerequest);

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

void OnPropertyNotify(XEvent *e) {

  EventMapping eventMap[] = {
      {XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False),
       updatePanelInfo},
      {XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False),
       updatePanelInfo},
      {XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False),
       updatePanelInfo},
  };
  XPropertyEvent *ev = &e->xproperty;
  size_t numEvents = sizeof(eventMap) / sizeof(EventMapping);
  Atom atom = ev->atom;
  for (size_t i = 0; i < numEvents; ++i) {
    if (eventMap[i].atom == atom) {
      EventMapping mapping = eventMap[i];
      mapping.handler(e);
      break;
    }
  }
  updatePanelInfo(e);
}

void Unframe(XUnmapEvent *ev) {
  XSync(display, False);
  struct Windows *tmp = getNextWindow(ev->window, MonitorIndex);
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

  // Extract information from the mapNotify event
  Window mappedWindow = mapEvent->window;

  if (mappedWindow == Root || mappedWindow == None) {
    return;
  }
  // Other properties can be accessed using mapEvent->property

  // Perform actions or updates based on the mapped window
  printf("Window mapped: %lu\n", mappedWindow);

  // Add your own code here to handle the mapNotify event

  // Example: Set the window title
  XStoreName(display, mappedWindow, "Mapped Window");

  // Example: Repaint the window
  XClearWindow(display, mappedWindow);
}

void OnDestroyNotify(XEvent *event) {
  XDestroyWindowEvent *destroyEvent = &event->xdestroywindow;
  if (destroyEvent->window == Root || destroyEvent->window == None) {
    return;
  }

  printf("Window with ID %lu is being destroyed.\n", destroyEvent->window);
  // Perform any cleanup or necessary actions here
  // ...

  // Reparent the window to the root window
  XReparentWindow(display, destroyEvent->window, Root, 0, 0);

  // Map the reparented window
  XMapWindow(display, destroyEvent->window);
}

void Frame(Window w) {
  if (w == Root || w == 0) {
    return;
  }
  int xpanel = 0;
  int ypanel = 0;
  XWindowAttributes x_window_attrs;
  XGetWindowAttributes(display, w, &x_window_attrs);
  XSetWindowAttributes wa;
  int x, y, z, v;
  unsigned int mask;
  Window root_return, child_return;
  XQueryPointer(display, w, &root_return, &child_return, &z, &v, &x, &y, &mask);
  XSelectInput(display, w,
               EnterWindowMask | FocusChangeMask | PropertyChangeMask |
                   StructureNotifyMask);
  XAddToSaveSet(display, w);
  XMapWindow(display, w);
  addWindow(w, MonitorIndex);
  if (panel.window != None) {
    xpanel = 0;
    ypanel = panel.height;
  }

  monitors[MonitorIndex].windows->x = xpanel;
  monitors[MonitorIndex].windows->y = ypanel;
  monitors[MonitorIndex].windows->width = x_window_attrs.width;
  monitors[MonitorIndex].windows->height = x_window_attrs.height;
  upWindow(w);
  wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask |
                  ButtonPressMask | PointerMotionMask | EnterWindowMask |
                  LeaveWindowMask | StructureNotifyMask | PropertyChangeMask;
  XChangeWindowAttributes(display, Root, CWEventMask | CWCursor, &wa);
  XSelectInput(display, Root, wa.event_mask);

  updateCurrentWindow(w, MonitorIndex);
  XRaiseWindow(display, w);
  XSetInputFocus(display, w, RevertToPointerRoot, CurrentTime);
  // Set the _NET_WM_ICON property
  unsigned char iconData[] = {
      // Icon pixel data
      0x00, 0x00, 0x00, 0x00, // Example pixel values
      0xFF, 0xFF, 0xFF, 0xFF, // Example pixel values
                              // ... more pixel values
  };

  unsigned int iconWidth = 32;
  unsigned int iconHeight = 32;
  Pixmap iconPixmap =
      createPixmap(display, RootWindow(display, DefaultScreen(display)),
                   iconWidth, iconHeight, iconData);
  XChangeProperty(display, w, XInternAtom(display, "_NET_WM_ICON", False),
                  XA_CARDINAL, 32, PropModeReplace, iconData, sizeof(iconData));

  setMotifWMHints(display, w, motifHints,
                  sizeof(motifHints) / sizeof(motifHints[0]));

  Atom atom_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
  setWindowProperty(display, w, "_NET_WM_WINDOW_TYPE", atom_type);
}
void setMotifWMHints(Display *display, Window window, unsigned long *hints,
                     int numHints) {
  Atom motifHintsAtom = XInternAtom(display, "_MOTIF_WM_HINTS", False);
  XChangeProperty(display, window, motifHintsAtom, motifHintsAtom, 32,
                  PropModeReplace, (unsigned char *)hints, numHints);
}

void setWindowProperty(Display *display, Window window,
                       const char *propertyName, Atom propertyValue) {
  Atom atom_property = XInternAtom(display, propertyName, False);
  XChangeProperty(display, window, atom_property, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)&propertyValue, 1);
  XSync(display, False);
}

Pixmap createPixmap(Display *display, Window root, unsigned int width,
                    unsigned int height, unsigned char *data) {
  Pixmap pixmap;
  Visual *visual;
  int depth;

  visual = DefaultVisual(display, DefaultScreen(display));
  depth = DefaultDepth(display, DefaultScreen(display));

  pixmap = XCreatePixmapFromBitmapData(display, root, (char *)data, width,
                                       height, 0, 1, depth);
  return pixmap;
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
  Atom atom_property = XInternAtom(display, "_NET_WM_STATE", False);
  Atom atom_type;
  int atom_format;
  unsigned long num_items, bytes_after;
  unsigned char *prop_value = NULL;

  getWindowProperty(display, window, "_NET_WM_STATE", "ATOM",
                    (void **)&prop_value, &num_items);

  if (prop_value != NULL) {
    Atom *atoms = (Atom *)prop_value;
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
  unsigned long num_items, bytes_after;
  unsigned char *prop_value = NULL;

  getWindowProperty(display, window, "_NET_WM_WINDOW_TYPE", "ATOM",
                    (void **)&prop_value, &num_items);
  if (prop_value != NULL) {
    Atom *atoms = (Atom *)prop_value;
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

int main() {
  dummyClient();
  initRootWidow();
  run();
  return 0;
}
