#include "X11/XKBlib.h"
#include "X11/Xlib.h"
#include "X11/Xproto.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/XF86keysym.h>
#include <sys/cdefs.h>

struct Windows{
  int identefire;
  int x,y,width,height,oldx,oldy,oldwidth,oldheight;
  int window;
  struct Windows *last;
};

struct Monitor{
  char **name;
  struct Windows *windows;
};

struct Monitor monitors[9];

bool addWindow(int window,int Index)
{
  if(Index <0 || Index > 9) return false;
  struct Windows *windows = malloc(sizeof(struct Windows));
  windows->last = monitors[Index].windows;
  windows->window = window;
  windows->identefire = -1;
  monitors[Index].windows = windows;
  return true;
}

bool removeWindow(int window,int Index)
{
  if(Index < 0 || Index > 9) return false;
  for(struct Windows **windows = &monitors[Index].windows; windows && (*windows)->window; windows = &(*windows)->last){
    if((*windows)->window == window){
      *windows = (*windows)->last;
      return true;
    }
  }
  return false;
}

void dummyClient(void) {
  for (int i = 0; i < 10; i++) {
    monitors[i].windows = malloc(sizeof(struct Windows));
    monitors[i].windows->last = malloc(sizeof(struct Windows));
  }
}

void getWindowsData()
{
  for(int i = 0 ; i< 10 ; i++){
    printf("%d : {   windows :",i);
    for(struct Windows *windows = monitors[i].windows ; windows && windows->window; windows = windows->last){
      printf("   %d : %d",windows->window,windows->identefire);
    }
    printf("  },\n");
  }
}

bool updateWindow(struct Windows *windows,int Index)
{
  if(Index < 0 || Index > 9) return false;
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
  if(Index < 0 || Index > 9 || NextIndex < 0 || NextIndex > 9) return false;
  struct Windows **tmp;
  struct Windows **next;
  for(tmp = &monitors[Index].windows ;tmp && (*tmp)->window && (*tmp)->last; tmp = &(*tmp)->last){
    if(window == (*tmp)->window){
      monitors[NextIndex].windows->last = monitors[NextIndex].windows;
      monitors[NextIndex].windows = *tmp;
      *tmp = (*tmp)->last;
      return true;
    }
  }
  return false;
}

int main(){
  dummyClient();
  addWindow(1, 1);
  addWindow(10, 1);
  addWindow(6, 1);
  addWindow(7, 3);
  addWindow(8, 4);
  addWindow(9, 5);
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
