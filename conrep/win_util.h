#ifndef CONREP_WINUTIL_H
#define CONREP_WINUTIL_H

#include "windows.h"

namespace console {
  void set_wndproc(HWND hWnd, WNDPROC proc);
  void set_userdata(HWND hWnd, void * user_data);
  
  void set_z_top(HWND hWnd);
  void set_z_normal(HWND hWnd);
  void set_z_bottom(HWND hWnd);
}

#endif