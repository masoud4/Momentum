
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
#define CLEAN_MASK(mask) ((mask) & ~(numlockmask | LockMask))
#define MAX_MONITOR 10
#define MAX_ARG 10
#define MAX_LENGTH 1024
#define GET_CURRENT_WINDOW(Index) monitors[Index].current->window
#define BUTTON_MASK (ButtonPressMask | ButtonReleaseMask)
#define MOUSE_MASK (BUTTON_MASK | PointerMotionMask)
#define MWM_HINTS_FLAGS_FIELD 0
#define MWM_HINTS_DECORATIONS_FIELD 2
#define WM_ATRIB                                                               \
  SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask |        \
      PointerMotionMask | EnterWindowMask | LeaveWindowMask |                  \
      StructureNotifyMask | PropertyChangeMask | Button1MotionMask |           \
      Button2MotionMask

#define MWM_HINTS_DECORATIONS (1 << 1)
#define MWM_DECOR_ALL (1 << 0)
#define MWM_DECOR_BORDER (1 << 1)
#define MWM_DECOR_TITLE (1 << 3)

#define WINDOW_IS_NOT_VALID window == Root || window == None
enum {
  ClkTagBar,
  ClkLtSymbol,
  ClkStatusText,
  ClkWinTitle,
  ClkClientWin,
  ClkRootWin,
  ClkLast
}; /* clicks */

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

typedef struct {
  unsigned int click;
  unsigned int mask;
  unsigned int button;
  void (*func)(const Arg *arg);
  const Arg arg;
} Button;

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

typedef struct {
  Window rootWindow;
  Window childWindow;
  int rootX, rootY, winX, winY;
  unsigned int mask;
  int screenNumber;
} info;

struct client {
  int identifier;
  int x, y, width, height, oldx, oldy, oldwidth, oldheight;
  state status;
  Window window;
  struct client *last;
};

struct panel {
  Window window;
  int position; // 0 : top  1,2,3 : right, button, left
  int height, width;
} panel;

struct Monitor {
  char **name;
  struct client *clients;
  struct client *current;
};

typedef struct {
  Atom atom;
  void (*handler)(XEvent *);
} EventMapping;

static void execute(const Arg *arg);

static void getWindowsData();

static void SwitchClients();

static void FullScreen();

static void Maximize();

static void resize(const Arg *arg);

static void move(const Arg *arg);

static void focus(const Arg *arg);

static info query(Window w);

static void SwitchMonitor(const Arg *arg);

static void MoveToMonitor(const Arg *arg);

static void KillWindow();

static void Arrange(const Arg *arg);

static int OnXError(Display *display, XErrorEvent *e);

static void grabkeys(void);

static int sendevent(Window w, Atom proto);

static bool initRootWidow(void);

static void OnMapRequest(XEvent *e);

static void Frame(Window window);

static void OnUnmapNotify(XEvent *e);

static void OnmapNotify(XEvent *e);

static void OnPropertyNotify(XEvent *e);

static void OnDestroyNotify(XEvent *e);

static void Unframe(XUnmapEvent *ev);

static void setFocus(XEvent *e);

static void justprint(XEvent *e);

static void onConfigureNotify(XEvent *e);

static void onConfigureRequest(XEvent *e);

static void onMotionNotify(XEvent *e);

static void Onkey(XEvent *e);

static void buttonpress(XEvent *e);

static void OnMapingNotify(XEvent *e);
static bool updateCurrentWindow(Window window, int Index);

static bool windowExist(Window window, int Index);

static void upWindow(Window window);

static void setMotifWMHints(Display *display, Window window,
                            unsigned long *hints, int numHints);

static void setWindowProperty(Display *display, Window window,
                              const char *propertyName, Atom propertyValue);

static void getWindowProperty(Display *display, Window window,
                              const char *propertyName, char *propertyType,
                              void **propertyValue, unsigned long *numItems);

static Pixmap createPixmap(Display *display, Window root, unsigned int width,
                           unsigned int height, unsigned char *data);

static bool getWindowAtom(Window window, char *AtomName);

static bool getWindow_NET_WM_Atom(Window window, char *AtomName);

static bool getWindow_EWMH_Atom(Window window, char *AtomName);

static struct client *getWindow(Window window, int Index);

static void run(void);

static bool isStickWindows(Window window);

static bool isAboveWindows(Window window);

static bool isPanel(Window window);

static void updatePanelInfo(XEvent *e);

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

char(*app[maxprogram][MAX_ARG]) = {
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
    {Mod1Mask, XK_Tab, SwitchClients, {}},
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
static Button buttons[] = {
    {ClkClientWin, Mod1Mask, Button1, resize, {}},
    {ClkClientWin, Mod4Mask, Button1, move, {}},
    {ClkClientWin, 0, Button1, focus, {}},
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
/*
 *

 */
void (*events[LASTEvent])(XEvent *e) = {
    [ClientMessage] = justprint,
    [ConfigureRequest] = onConfigureRequest,
    [MapNotify] = OnmapNotify,
    [DestroyNotify] = OnDestroyNotify,
    [FocusIn] = setFocus,
    [KeyPress] = Onkey,
    [ButtonPress] = buttonpress,
    [MapRequest] = OnMapRequest,
    [MotionNotify] = onMotionNotify,
    [PropertyNotify] = OnPropertyNotify,
    [UnmapNotify] = OnUnmapNotify,
/*
    [ConfigureNotify] = onConfigureNotify,
    [EnterNotify] = justprint,
    [Expose] = justprint,
    [ResizeRequest] = justprint,

    [MappingNotify] = justprint,
    [KeyRelease] = justprint,
    [ButtonRelease] = justprint,
    [LeaveNotify] = justprint,
    [FocusOut] = justprint,
    [KeymapNotify] = justprint,
    [GraphicsExpose] = justprint,
    [NoExpose] = justprint,
    [VisibilityNotify] = justprint,
    [CreateNotify] = justprint,
    [ReparentNotify] = justprint,
    [GravityNotify] = justprint,
    [CirculateNotify] = justprint,
    [CirculateRequest] = justprint,
    [SelectionClear] = justprint,
    [SelectionRequest] = justprint,
    [SelectionNotify] = justprint,
    [ColormapNotify] = justprint,
    [GenericEvent] = justprint,
    */
};

Display *display;
Window Root;
int MonitorIndex = 1;
Screen *screen;
static unsigned int numlockmask;

unsigned long motifHints[] = {
    MWM_HINTS_DECORATIONS, MWM_DECOR_ALL,
    MWM_HINTS_FLAGS_FIELD, MWM_HINTS_DECORATIONS_FIELD,
    MWM_HINTS_DECORATIONS, MWM_DECOR_BORDER,
    MWM_DECOR_TITLE,
};
