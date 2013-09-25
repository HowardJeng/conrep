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

// Interface for the console replacement window.

#ifndef CONREP_CONSOLE_WINDOW_H
#define CONREP_CONSOLE_WINDOW_H

#include <boost/shared_ptr.hpp>

#include "windows.h"
#include "tchar.h"

namespace console {
  class IDirect3DRoot;
  typedef boost::shared_ptr<IDirect3DRoot> RootPtr;

  enum WindowState {
    INITIALIZING,
    RUNNING,
    RESETTING,
    CLOSING,
    DEAD
  };

  struct __declspec(novtable) IConsoleWindow {
    virtual HWND get_hwnd(void) const = 0;
    virtual HWND get_console_hwnd(void) const = 0;

    virtual void dispose_resources(void) = 0;
    virtual void restore_resources(void) = 0;
    virtual WindowState get_state(void) const = 0; // for debugging

    virtual ~IConsoleWindow() = 0;
  };
  
  typedef boost::shared_ptr<IConsoleWindow> WindowPtr;

  struct Settings;
  
  WindowPtr create_console_window(HWND hub, 
                                  HINSTANCE hInstance, 
                                  const Settings & settings, 
                                  RootPtr root, 
                                  const tstring & exe_dir,
                                  tstring & message);
}

#endif