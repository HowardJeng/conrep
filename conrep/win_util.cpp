// win_util.cpp
// contains implementation for utility functions for dealing with Windows window classes
#include "win_util.h"
#include "exception.h"

namespace console {
  void set_wndproc(HWND hWnd, WNDPROC proc) {
    SetLastError(0);
    LONG_PTR ret_val = SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
    if (ret_val == 0) {
      DWORD err = GetLastError();
      if (err) WIN_EXCEPT2("Failure in SetWindowLongPtr(GWLP_WNDPROC).", err);
    }
  }

  void set_userdata(HWND hWnd, void * user_data) {
    SetLastError(0);
    LONG_PTR ret_val = SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(user_data));
    if (ret_val == 0) {
      DWORD err = GetLastError();
      if (err) WIN_EXCEPT2("Failure in SetWindowLongPtr(GWLP_USERDATA).", err);
    }
  }


  void set_z_top(HWND hWnd) {
    if (!SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) WIN_EXCEPT("Failed call to SetWindowPos(). ");
  }

  void set_z_normal(HWND hWnd) {
    if (!SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) WIN_EXCEPT("Failed call to SetWindowPos(). ");
  }

  void set_z_bottom(HWND hWnd) {
    if (!SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) WIN_EXCEPT("Failed call to SetWindowPos(). ");
  }

}  
