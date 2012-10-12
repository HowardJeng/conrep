// Interface for the console replacement window.

#ifndef CONREP_CONSOLE_WINDOW_H
#define CONREP_CONSOLE_WINDOW_H

#include <boost/shared_ptr.hpp>

#include "windows.h"

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
  
  WindowPtr create_console_window(HWND hub, HINSTANCE hInstance, const Settings & settings, RootPtr root);
}

#endif