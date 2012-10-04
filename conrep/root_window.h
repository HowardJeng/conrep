// Interface for the root/hub window
#ifndef CONREP_ROOT_WINDOW_H
#define CONREP_ROOT_WINDOW_H

#include "windows.h"
#include <memory>
#include "tchar.h"

namespace console {
  struct Settings;

  class __declspec(novtable) IRootWindow {
    public:
      virtual ~IRootWindow() = 0;
      virtual bool spawn_window(LPCTSTR command_line) = 0;
      virtual bool spawn_window(const Settings & settings) = 0;
      virtual HWND hwnd(void) const = 0;
  };

  std::auto_ptr<IRootWindow> get_root_window(HINSTANCE hInstance, const tstring & exe_dir);

}

#endif