
#include "X11/X.h"
#include "X11/Xlib.h"
#include "X11/Xproto.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <unistd.h>

#define LENGTH(x) (sizeof(x) / sizeof(*(x)))
#define CLEANMASK(mask) ((mask) & ~(numlockmask | LockMask))
#define MAXMONITOR 10
#define MAXARG 10
#define MAXLENGHT 1024
#define GETCURRENTWINDOW(Index) monitors[Index].current->window
#define SETCURRENTWINDOWS(Index, windows) monitors[Index].current = windows

#define ROOT_MASK SubstructureRedirectMask | SubstructureNotifyMask

#define NORMAL_WINDOW_MASK                                                     \
  EnterWindowMask | LeaveWindowMask | PointerMotionHintMask |                  \
      FocusChangeMask | PropertyChangeMask | PointerMotionMask
#define BUTTON_MASK SubstructureRedirectMask | SubstructureNotifyMask

#define M_LOG true
#if M_LOG
#define _Xdebug 1
#define log(msg, window) printf("[ %s ] \t,%lu\n", msg, window)
#else
#define log(msg, window)
#endif // M_LOG

#define MWM_HINTS_FLAGS_FIELD 0
#define MWM_HINTS_DECORATIONS_FIELD 2

#define MWM_HINTS_DECORATIONS (1 << 1)
#define MWM_DECOR_ALL (1 << 0)
#define MWM_DECOR_BORDER (1 << 1)
#define MWM_DECOR_TITLE (1 << 3)

typedef union {
  const char **com;
  const int i;
  const void *v;
} Arg;

typedef struct {
  unsigned int mod;
  KeySym keysym;
  void (*func)(const Arg *);
  const Arg arg;
} Key;

typedef enum {
  STICKY,
  MAXIMIZED_VERT,
  MAXIMIZED_HORZ,
  SHADED,
  SKIP_TASKBAR,
  SKIP_PAGER,
  HIDDEN,
  FULLSCREEN,
  ABOVE,
  BELOW,
  DEMANDS_ATTENTION,
} state;

struct Windows {
  int identefire;
  int x, y, width, height, oldx, oldy, oldwidth, oldheight;
  state status;
  Window window;
  struct Windows *last;
};

struct panel {
  Window window;
  int position; // 0 means top  1,2,3 means right, button, left
  int height, width;
} panel;

struct Monitor {
  char **name;
  struct Windows *windows;
  struct Windows *current;
  int vericalEdge;
};

typedef struct {
  Atom atom;
  void (*handler)(XEvent *);
} EventMapping;

void execute(const Arg *arg);
void getWindowsData();
void SwitchWindows();
void FullScreen();
void Maximize();
void SwitchMonitor(const Arg *arg);
void MoveToMonitor(const Arg *arg);
void KillWindow();
void Arrange(const Arg *arg);
int OnXError(Display *display, XErrorEvent *e);
void grabkeys(void);
int sendevent(Window w, Atom proto);
bool initRootWidow(void);
void OnMapRequest(XEvent *e);
void Frame(Window window);
void OnUnmapNotify(XEvent *e);
void OnmapNotify(XEvent *e);
void OnPropertyNotify(XEvent *e);
void OnDestroyNotify(XEvent *e);
void OnConfigureNotify(XEvent *e);

void OnMappingNotify(XEvent *e);

void Unframe(XUnmapEvent *ev);
void setFocus(XEvent *e);
void justprint(XEvent *e);
void onConfigureRequest(XEvent *e);
void onMotionNotify(XEvent *e);
void Onkey(XEvent *e);
bool updateCurrentWindow(Window window, int Index);
bool windowExist(Window window, int Index);
void upWindow(Window window);

void setWindowEvent(Window w);
void setRootEvent();
// TODO :this is just some idea
// void resize(Window window);
// void move(Window window);
// tiling window support
// lua integration for each parts. it could be good for feathure
// void sendNotefication(Window window,char ** msg,int up);
// create clickable {button menue labels textbox}

void setMotifWMHints(Display *display, Window window, unsigned long *hints,
                     int numHints);

void setWindowProperty(Display *display, Window window,
                       const char *propertyName, Atom propertyValue);

void getWindowProperty(Display *display, Window window,
                       const char *propertyName, char *propertyType,
                       void **propertyValue, unsigned long *numItems);

Pixmap createPixmap(Display *display, Window root, unsigned int width,
                    unsigned int height, unsigned char *data);

bool getWindowAtom(Window window, char *AtomName);
bool getWindow_NET_WM_Atom(Window window, char *AtomName);
bool getWindow_EWMH_Atom(Window window, char *AtomName);
struct Windows *getWindow(Window window, int Index);
void run(void);
bool isStickWindows(Window window);
bool isAboveWindows(Window window);
bool isPanel(Window window);
void updatePanelInfo(XEvent *e);

enum programs {
  SoundStart,
  SoundTerminate,
  SoundWithdrew,
  terminal,
  menu,
  filemanager,
  volup,
  voldown,
  changebackgraound,
  browser,
  editor,
  screenshot,
  record_screen,
  playmusic,
  killmusic,
  getvolume,
  search,

  maxprogram
};
char(*app[maxprogram][MAXARG]) = {
    [SoundStart] = {"mplayer",
                    "/usr/local/share/sounds/macOSBigSurSound/Bottle.aiff",
                    NULL},
    [SoundTerminate] = {"mplayer",
                        "/usr/local/share/sounds/macOSBigSurSound/Pop.aiff",
                        NULL},
    [SoundWithdrew] = {"mplayer",
                       "/usr/local/share/sounds/macOSBigSurSound/Pop.aiff",
                       NULL},

    [terminal] = {"xfce4-terminal", NULL},
    [menu] = {"dmenu_run", NULL},
    [filemanager] = {"pcmanfm", NULL},
    [volup] = {"mixer", "-S", "vol", "+2", NULL},
    [voldown] = {"mixer", "-S", "vol", "-2", NULL},
    [changebackgraound] = {"fbsetbg", "-r", "/mnt/other/Wallpaper", NULL},
    [browser] = {"firefox", NULL},
    [editor] = {"xfce4-terminal", "-e", "nvim", NULL},
    [screenshot] = {"Naqsh", NULL},
    [record_screen] = {"recordAllfeature.sh", NULL},
    [playmusic] = {"mplayer", "-shuffle", "-playlist",
                   "/usr/home/Masoud/Desktop/Music/programing/playlist.txt",
                   NULL},
    [killmusic] = {"killall", "mplayer", NULL},
    [getvolume] = {"mixer", "vol", NULL},
    [search] = {"xfce4-terminal", "-e",
                "fzf --preview \"bat --color=always --style=numbers "
                "--line-range=:500 {}\"",
                NULL},

};

static Key keys[] = {
    {Mod4Mask, XK_r, execute, {.v = &app[terminal]}},
    {Mod4Mask, XK_p, execute, {.v = &app[menu]}},
    {Mod4Mask, XK_a, execute, {.v = &app[editor]}},
    {Mod4Mask, XK_e, execute, {.v = &app[filemanager]}},
    {Mod4Mask, XK_z, execute, {.v = &app[changebackgraound]}},
    {Mod4Mask, XK_o, execute, {.v = &app[browser]}},
    {Mod4Mask, XK_s, execute, {.v = &app[record_screen]}},
    {Mod4Mask, XK_Print, execute, {.v = &app[screenshot]}},
    {Mod4Mask, XK_m, execute, {.v = &app[playmusic]}},
    {Mod4Mask, XK_q, execute, {.v = &app[killmusic]}},
    {0, XK_F2, execute, {.v = &app[voldown]}},
    {0, XK_F3, execute, {.v = &app[volup]}},
    {Mod1Mask, XK_l, getWindowsData, {}},
    {Mod1Mask, XK_Tab, SwitchWindows, {}},
    {Mod1Mask, XK_f, Maximize, {}},
    {Mod1Mask, XK_F11, FullScreen, {}},
    {Mod1Mask, XK_F4, KillWindow, {}},
    {ControlMask, XK_slash, execute, {.v = &app[search]}},
    {Mod4Mask, XK_g, Arrange, {.i = 0}},
    {Mod4Mask, XK_h, Arrange, {.i = 1}},
    {Mod4Mask, XK_0, SwitchMonitor, {.i = 0}},
    {Mod4Mask, XK_1, SwitchMonitor, {.i = 1}},
    {Mod4Mask, XK_2, SwitchMonitor, {.i = 2}},
    {Mod4Mask, XK_3, SwitchMonitor, {.i = 3}},
    {Mod4Mask, XK_4, SwitchMonitor, {.i = 4}},
    {Mod4Mask, XK_5, SwitchMonitor, {.i = 5}},
    {Mod4Mask, XK_6, SwitchMonitor, {.i = 6}},
    {Mod4Mask, XK_7, SwitchMonitor, {.i = 7}},
    {Mod4Mask, XK_8, SwitchMonitor, {.i = 8}},
    {Mod4Mask, XK_9, SwitchMonitor, {.i = 9}},
    {Mod4Mask | ShiftMask, XK_1, MoveToMonitor, {.i = 1}},
    {Mod4Mask | ShiftMask, XK_2, MoveToMonitor, {.i = 2}},
    {Mod4Mask | ShiftMask, XK_3, MoveToMonitor, {.i = 3}},
    {Mod4Mask | ShiftMask, XK_4, MoveToMonitor, {.i = 4}},
    {Mod4Mask | ShiftMask, XK_5, MoveToMonitor, {.i = 5}},
    {Mod4Mask | ShiftMask, XK_6, MoveToMonitor, {.i = 6}},
    {Mod4Mask | ShiftMask, XK_7, MoveToMonitor, {.i = 7}},
    {Mod4Mask | ShiftMask, XK_8, MoveToMonitor, {.i = 8}},
    {Mod4Mask | ShiftMask, XK_9, MoveToMonitor, {.i = 9}},
    {Mod4Mask | ShiftMask, XK_0, MoveToMonitor, {.i = 0}},
};

char(*errors[BadImplementation + 1]) = {
    [Success] = "everything's okay ",
    [BadRequest] = "bad request code ",
    [BadValue] = "int parameter out of range ",
    [BadWindow] = "parameter not a Window ",
    [BadPixmap] = "parameter not a Pixmap ",
    [BadAtom] = "parameter not an Atom ",
    [BadCursor] = "parameter not a Cursor ",
    [BadFont] = "parameter not a Font ",
    [BadMatch] = "parameter mismatch ",
    [BadDrawable] = "parameter not a Pixmap or Window ",
    [BadAccess] = "depending on context",
    [BadAlloc] = "insufficient resources ",
    [BadColor] = "no such colormap ",
    [BadGC] = "parameter not a GC ",
    [BadIDChoice] = "choice not in range or already used ",
    [BadName] = "font or color name doesn't exist ",
    [BadLength] = "Request length incorrect ",
    [BadImplementation] = "server is defective ",
};

void (*events[LASTEvent])(XEvent *e) = {
    [CirculateNotify] = justprint,
    [CirculateRequest] = justprint,
    [GenericEvent] = justprint,
    [ButtonPress] = justprint,
    [ButtonRelease] = justprint,
    [ClientMessage] = justprint,
    [ColormapNotify] = justprint,
    [ConfigureNotify] = OnConfigureNotify,
    [ConfigureRequest] = onConfigureRequest,
    [CreateNotify] = justprint,
    [DestroyNotify] = OnDestroyNotify,
    [EnterNotify] = justprint,
    [LeaveNotify] = justprint,
    [Expose] = justprint,
    [FocusIn] = justprint,
    [FocusOut] = justprint,
    [GraphicsExpose] = justprint,
    [GravityNotify] = justprint,
    [KeymapNotify] = justprint,
    [KeyPress] = Onkey,
    [KeyRelease] = justprint,
    [MapNotify] = OnmapNotify,
    [MapRequest] = OnMapRequest,
    [MappingNotify] = OnMappingNotify,
    [MotionNotify] = onMotionNotify,
    [NoExpose] = justprint,
    [PropertyNotify] = OnPropertyNotify,
    [ReparentNotify] = justprint,
    [ResizeRequest] = justprint,
    [SelectionClear] = justprint,
    [SelectionNotify] = justprint,
    [SelectionRequest] = justprint,
    [UnmapNotify] = OnUnmapNotify,
    [VisibilityNotify] = justprint,
};

Display *display;
Window Root;
int MonitorIndex = 1;
int screen;
static unsigned int numlockmask;

unsigned long motifHints[] = {
    MWM_HINTS_DECORATIONS, MWM_DECOR_ALL,
    MWM_HINTS_FLAGS_FIELD, MWM_HINTS_DECORATIONS_FIELD,
    MWM_HINTS_DECORATIONS, MWM_DECOR_BORDER,
    MWM_DECOR_TITLE,
};
