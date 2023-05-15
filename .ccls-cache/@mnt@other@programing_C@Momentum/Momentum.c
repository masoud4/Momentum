#include "Momentum.h"
#include "X11/X.h"
#include "X11/Xproto.h"

struct Monitor monitors[MAXMONITOR];

bool addWindow(int window,int Index)
{
  if(Index <0 || Index > MAXMONITOR) return false;;
  struct Windows *windows = malloc(sizeof(struct Windows));
  windows->last = monitors[Index].windows;
  windows->window = window;
  windows->identefire = -1;
  monitors[Index].windows = windows;
  updateCurrentWindow(window,Index);
  return true;
}

bool removeWindow(int window,int Index)
{
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
    printf("%d : { current : %d,  windows :",i,monitors[i].current->window);
    for(struct Windows *windows = monitors[i].windows ; windows && windows->window; windows = windows->last){
      printf("   %d : %d",windows->window,windows->identefire);
    }
    printf("  },\n");
  }

}
bool updateCurrentWindow(int window, int Index)
{
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

bool modifyMonitor(int window,int Index,int NextIndex){
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
    fprintf(stderr, "Moon: Execute %s", ((char **)arg->v)[0]);
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

int OnXError(Display *display, XErrorEvent *e) {
  char error_text[MAXLENGHT];
  XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
  printf("\t[EE] %s\n\t[EE]%s\n", errors[e->error_code], error_text);
  return 1;
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

struct Windows *getNextWindow(int window, int Index) {
  for (struct Windows *w = monitors[Index].windows; w && w->last ; w = w->last) {
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
void upWindow(int window) {
  if (window == Root) {
    return;
  }
  XRaiseWindow(display, window);
  struct Windows *windows = getWindow(window, MonitorIndex);
  if (windows != NULL) {
    XWindowChanges changes;
    changes.x = windows->oldx;
    changes.y = windows->oldy;
    changes.width = windows->oldwidth;
    changes.height = windows->oldheight;
    changes.border_width = 0;
    changes.sibling = None;
    changes.stack_mode = Above;
    XConfigureWindow(display, window, CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &changes);
  }
  XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
}

bool windowExist(int window, int Index) 
{
  XWindowAttributes attrs;
  return XGetWindowAttributes(display, window, &attrs);
}

struct Windows* getWindow(int window, int Index)
{
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
  struct Windows *windows = getNextWindow(current,MonitorIndex);
  XMoveResizeWindow(display,current , 0, 0,
      max_width , max_height);
  XSync(display, False);
}

void SwitchMonitor(const Arg *arg)
{
if (arg->i == MonitorIndex || arg->i < 0 || arg->i > MAXMONITOR) {
        return;
    }
    struct Monitor *d = &monitors[MonitorIndex];
    struct Monitor *n = &monitors[(MonitorIndex = arg->i)];
    // Map all clients in the new workspace and bring them to the top
    for (struct Windows *c = n->windows; c && c->last; c = c->last) {
        XMapWindow(display, c->window);
        upWindow(c->window);
    }

    // Disable SubstructureNotify events to prevent them from being propagated
    XSetWindowAttributes attr = {.do_not_propagate_mask = SubstructureNotifyMask};
    XChangeWindowAttributes(display, Root, CWEventMask, &attr);

    // Unmap all clients in the old workspace
    for (struct Windows *c = d->windows; c && c->last; c = c->last) {
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
}

void MoveToMonitor(const Arg *arg){
    if (arg->i == MonitorIndex || arg->i < 0 || arg->i > MAXMONITOR) {
        return;
    }
    int window = GETCURRENTWINDOW(MonitorIndex);
     modifyMonitor(window, MonitorIndex, arg->i);
}
void KillWindow()
{
    int window = GETCURRENTWINDOW(MonitorIndex);

}
int main(){
  dummyClient();
  initRootWidow();
  addWindow(1, 1);
  addWindow(10, 1);
  addWindow(6, 1);
  addWindow(7, 3);
  addWindow(8, 4);
  addWindow(9, 5);
  addWindow(9, 2);
  addWindow(-1, 1);
  removeWindow(-1, 1);
  struct Windows *win;
  addWindow(11, 1);
  win->window = 112;
  win->identefire = 8;
  updateWindow(win, 1);
  modifyMonitor(7, 3, 0);
  printf("\n");
 getWindowsData();
  return 0;
}
