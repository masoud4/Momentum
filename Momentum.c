#include "Momentum.h"
#include "X11/X.h"
#include "X11/Xproto.h"

struct Monitor monitors[MAXMONITOR];

bool addWindow(Window window,int Index)
{
  if (window == Root || window == 0) {
    return false;
  }
  if(Index <0 || Index > MAXMONITOR) return false;;
  struct Windows *windows = malloc(sizeof(struct Windows));
  windows->last = monitors[Index].windows;
  windows->window = window;
  windows->identefire = -1;
  monitors[Index].windows = windows;
  updateCurrentWindow(window,Index);
  return true;
}

bool removeWindow(Window window,int Index)
{
  if (window == Root || window == 0) {
    return false;
  }
  if(Index < 0 || Index > MAXMONITOR) return false;
  for(struct Windows **windows = &monitors[Index].windows; windows && (*windows)->window; windows = &(*windows)->last){
    if((*windows)->window == window){
      *windows = (*windows)->last;
      updateCurrentWindow(-1,Index);
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
}

void getWindowsData()
{
    for(int i = 0 ; i<= MAXMONITOR ; i++){
      Window current = monitors[i].current != (struct Windows*)NULL ? monitors[i].current->window : (Window)-1;
      printf("%d : { current : %lu,  windows :",i,current);
      if(monitors[i].current->window != (Window)NULL){
        for(struct Windows *windows = monitors[i].windows ; windows && windows->window ; windows = windows->last){
          printf("   %lu : %d",windows->window,windows->identefire);
        }
      }
      printf("  },\n");
    }

}
bool updateCurrentWindow(Window window, int Index)
{
  if (window == Root || window == 0) {
    return false;
  }
  if(Index < 0 || Index > MAXMONITOR) return false;
  for(struct Windows *windows = monitors[Index].windows ; windows && windows->window; windows = windows->last){
    if(windows->window == window){
      monitors[Index].current = windows;
      return true;
    }
  }
  monitors[Index].current = monitors[Index].windows;
  return false;
}

bool updateWindow(struct Windows *windows,int Index)
{
  if (windows->window == Root || windows->window == 0) {
    return false;
  }
  if(Index < 0 || Index > MAXMONITOR) return false;
  for(struct Windows **tmp = &monitors[Index].windows ; tmp && (*tmp)->window; tmp = &(*tmp)->last){
    if(windows->window == (*tmp)->window){
      windows->last = (*tmp)->last;
      *tmp = windows;  
      return true;
    }
  }   
  return false;
}

bool modifyMonitor(Window window,int Index,int NextIndex){
  if (window == Root || window == 0) {
    return false;
  }
  if(Index < 0 || Index > MAXMONITOR || NextIndex < 0 || NextIndex > MAXMONITOR) return false;
  struct Windows **tmp;
  struct Windows **next;
  for(tmp = &monitors[Index].windows ;tmp && (*tmp)->window && (*tmp)->last; tmp = &(*tmp)->last){
    if(window == (*tmp)->window){
      monitors[NextIndex].windows->last = monitors[NextIndex].windows;
      monitors[NextIndex].windows = *tmp;
      *tmp = (*tmp)->last;
      if(monitors[Index].current->window == window){
        updateCurrentWindow(-1,Index);
      }
      updateCurrentWindow(window,NextIndex);
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

bool initRootWidow(void)
{
  display = XOpenDisplay(NULL);
  if (!display) {
    fprintf(stderr, "Failed to open X display\n");
    return False;
  }
  Root = DefaultRootWindow(display);
  screen = DefaultScreen(display);
  XSetErrorHandler(OnXError);
  XGrabServer(display);
  XSync(display, False);
  XSelectInput(display, Root,
      SubstructureRedirectMask | SubstructureNotifyMask);
  XUngrabServer(display);
  grabkeys();
  MonitorIndex = 0;
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
  if (window == 0) {
  return monitors[Index].windows;
  }
  for (struct Windows *w = monitors[Index].windows; w && w->last->window ; w = w->last) {
    if (w->window == window) {
      return w->last;
    }
  }
  return monitors[Index].windows;
}

void SwitchWindows()
{
  struct Windows *tmp = getNextWindow(GETCURRENTWINDOW(MonitorIndex),MonitorIndex);
  if(tmp && windowExist(tmp->window,  MonitorIndex)){
    upWindow(tmp->window);
  }
}
void upWindow(Window window) {
  if (window == Root || window == 0) {
    return ;
  }
  XRaiseWindow(display, window);
  struct Windows *windows = getWindow(window, MonitorIndex);
  XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
  Atom atom_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
  if (atom_above != None) {
      XChangeProperty(display, window, XInternAtom(display, "_NET_WM_STATE", False),
                      XA_ATOM, 32, PropModeReplace, (unsigned char *)&atom_above, 1);
  }
  updateCurrentWindow(window, MonitorIndex);
}

bool windowExist(Window window, int Index) 
{
  if (window == Root || window == 0) {
    return false;
  }
  XWindowAttributes attrs;
  return XGetWindowAttributes(display, window, &attrs);
}

struct Windows* getWindow(Window window, int Index)
{
  if (window == Root || window == 0) {
    return NULL;
  }
  for(struct Windows *tmp = monitors[Index].windows; tmp && tmp->window; tmp = tmp->last){
    if(tmp->window == window){
      return tmp;
    }
  }
  return NULL;
}

void FullScreen()
{

    int current = GETCURRENTWINDOW(MonitorIndex);
    int screen = DefaultScreen(display);
    int max_width = DisplayWidth(display, screen);
    int max_height = DisplayHeight(display, screen);

    struct Windows *windows = getNextWindow(current, MonitorIndex);
    XMoveResizeWindow(display, current, 0, 0, max_width, max_height);

    Atom atom_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    if (atom_fullscreen != None) {
        XChangeProperty(display, current, XInternAtom(display, "_NET_WM_STATE", False),
                        XA_ATOM, 32, PropModeReplace, (unsigned char *)&atom_fullscreen, 1);
    }

    XSync(display, False);
}
void SwitchMonitor(const Arg *arg)
{
  if (arg->i == MonitorIndex || arg->i < 0 || arg->i > MAXMONITOR) {
    return;
  }
  struct Monitor *d = &monitors[MonitorIndex];
  struct Monitor *n = &monitors[ arg->i];
  MonitorIndex = arg->i;
  // Map all clients in the new workspace and bring them to the top
  for (struct Windows *c = n->windows; c && c->window ; c = c->last) {
    XMapWindow(display, c->window);
    upWindow(c->window);
  }

  // Disable SubstructureNotify events to prevent them from being propagated
  XSetWindowAttributes attr = {.do_not_propagate_mask = SubstructureNotifyMask};
  XChangeWindowAttributes(display, Root, CWEventMask, &attr);

  // Unmap all clients in the old workspace
  for (struct Windows *c = d->windows; c && c->window ; c = c->last) {
    XUnmapWindow(display, c->window);
  }

  // Enable the necessary events again
  attr.event_mask = SubstructureRedirectMask | ButtonPressMask | SubstructureNotifyMask | PropertyChangeMask;
  XChangeWindowAttributes(display, Root, CWEventMask, &attr);

  // Bring the current client to the top
  struct Windows *windows = getWindow(GETCURRENTWINDOW(MonitorIndex), MonitorIndex);
  if (windows != NULL) {
    upWindow(windows->window);
  }
    XSync(display, False);
}

void MoveToMonitor(const Arg *arg){
  if (arg->i == MonitorIndex || arg->i < 0 || arg->i > MAXMONITOR) {
    return;
  }
  Window window = GETCURRENTWINDOW(MonitorIndex);
  modifyMonitor(window, MonitorIndex, arg->i);
  XUnmapWindow(display, window);
    XSync(display, False);
}
void KillWindow()
{
  Window window = GETCURRENTWINDOW(MonitorIndex);

}
void run()
{
  while (True) {
    XEvent e;
    XNextEvent(display, &e);
    if (events[e.type]) {
      events[e.type](&e);
    }
  }
}

void Onkey(XEvent *e) {
  justprint(e);
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
void setFocus(XEvent *e) {
  XFocusChangeEvent *ev = &e->xfocus;
}

void OnUnmapNotify(XEvent *e) {
  XUnmapEvent *ev = &e->xunmap;

  bool exist = getWindow(ev->window, MonitorIndex) == NULL;
  if (!exist) {
    Unframe(ev);
  }
  XSync(display, False);
}

void Unframe(XUnmapEvent *ev) {
  XSync(display,False);
  struct Windows *tmp = getNextWindow(ev->window, MonitorIndex);
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
  if (!XGetWindowAttributes(display, ev->window, &wa)) {
    return;
  }
  if (wa.override_redirect) {
    return;
  }
  Frame(ev->window);
  XMapWindow(display, ev->window);
}

void Frame(Window w) {
  if (w == Root || w == 0) {
    return ;
  }
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
  monitors[MonitorIndex].windows->x = 0;
  monitors[MonitorIndex].windows->y = 0;
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
}
int main(){
  dummyClient();
  initRootWidow();
  run();
  return 0;
}
