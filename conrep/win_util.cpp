/* 
 * Copyright 2007-2013 Howard Jeng <hjeng@cowfriendly.org>
 * 
 * This file is part of Conrep.
 * 
 * Conrep is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * Eraser is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License can be found at
 * <http://www.gnu.org/licenses/>.
 */

// win_util.cpp
// contains implementation for utility functions for dealing with Windows window classes
#include "win_util.h"
#include "exception.h"

namespace console {
  void set_wndproc(HWND hWnd, WNDPROC proc, bool allow_invalid_handle) {
    SetLastError(0);
    LONG_PTR ret_val = SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
    if (ret_val == 0) {
      DWORD err = GetLastError();
      if (err && !((err == 1400) && allow_invalid_handle)) {
        WIN_EXCEPT2("Failure in SetWindowLongPtr(GWLP_WNDPROC).", err);
      }
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
    if (!SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) WIN_EXCEPT("Failed call to SetWindowPos(). ");
    if (!SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) WIN_EXCEPT("Failed call to SetWindowPos(). ");
  }

  void set_z_bottom(HWND hWnd) {
    if (!SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE)) WIN_EXCEPT("Failed call to SetWindowPos(). ");
  }

}  
