
#include "X11/Xlib.h"
#include "X11/Xproto.h"
#include "X11/X.h"
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/XF86keysym.h>
#include <sys/cdefs.h>
#include <X11/XKBlib.h>
#define LENGTH(x) (sizeof(x) / sizeof(*(x)))
#define CLEANMASK(mask) ((mask) & ~(numlockmask | LockMask))
#define MAXMONITOR 10
#define MAXARG 10
#define  MAXLENGHT 1024
#define GETCURRENTWINDOW(Index) monitors[Index].current->window
#define SETCURRENTWINDOWS(Index, windows) monitors[Index].current = windows

typedef union{
  const char **com;
  const int i;
  const void *v;
} Arg;

typedef struct{
  unsigned int mod;
  KeySym keysym;
  void (*func)(const Arg *);
  const Arg arg;
} Key;

struct Windows{
  int identefire;
  int x,y,width,height,oldx,oldy,oldwidth,oldheight;
  Window window;
  struct Windows *last;
};

struct Monitor{
  char **name;
  struct Windows *windows;
  struct Windows *current;
};



void execute(const Arg *arg);
void getWindowsData();
void SwitchWindows();
void FullScreen();
void SwitchMonitor(const Arg *arg);
void MoveToMonitor(const Arg *arg);
void KillWindow();
int OnXError(Display *display, XErrorEvent *e);
void grabkeys(void);
int sendevent(Window w, Atom proto);
bool initRootWidow(void);
void OnMapRequest(XEvent *e);
void Frame(Window window);
void OnUnmapNotify(XEvent *e);
void Unframe(XUnmapEvent *ev);
void setFocus(XEvent *e);
void justprint(XEvent *e) {}
void Onkey(XEvent *e);
bool updateCurrentWindow(Window window, int Index);
bool windowExist(Window window, int Index) ;
void upWindow(Window window);
struct Windows* getWindow(Window window, int Index);
void run(void);
char(*monitorsTags[MAXMONITOR+1]) = {
    [1] = "", [2] = "", [3] = "", [4] = "", [5] = "",
    [6] = "", [7] = "", [8] = "", [9] = "", [0] = ""};

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
    [editor] = {"gnome-terminal",
                " --window "
                " -- ",
                "vim", NULL},
    [screenshot] = {"printscreen", NULL},
    [record_screen] = {"recordAllfeature.sh", NULL},
    [playmusic] = {"mplayer", "-shuffle", "-playlist",
                   "/usr/home/Masoud/Desktop/Music/dream/playlist", NULL},
    [killmusic] = {"killall", "mplayer", NULL},
    [getvolume] = {"mixer", "vol", NULL},
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
    {Mod1Mask, XK_f, FullScreen, {}},
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
    {0, XK_F4, getWindowsData, {}},
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
    [ButtonPress] = justprint,
    [ClientMessage] = justprint,
    [ConfigureRequest] = justprint,
    [ConfigureNotify] = justprint,
    [DestroyNotify] = justprint,
    [EnterNotify] = justprint,
    [Expose] = justprint,
    [FocusIn] = setFocus,
    [KeyPress] = Onkey,
    [MappingNotify] = justprint,
    [MapRequest] = OnMapRequest,
    [MotionNotify] = justprint,
    [PropertyNotify] = justprint,
    [UnmapNotify] = OnUnmapNotify,
};

Display *display;
Window Root;
int MonitorIndex = 1 ;
int screen;
static unsigned int numlockmask;
