#include "X11/XKBlib.h"
#include "X11/Xlib.h"
#include "X11/Xproto.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/XF86keysym.h>

struct WindowsList{
  int x,y,width,height,oldx,oldy,oldwidth,oldheight;
  int window;
};

struct Monitor{
  int index;
  char *name;
  struct WindowsList *next;
  };

double addWindow(struct WindowsList * Window )
{
  return false;
}

int main(){
  return 0;
}
