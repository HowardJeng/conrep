// Interface for the root/hub window
#ifndef CONREP_ROOT_WINDOW_H
#define CONREP_ROOT_WINDOW_H

#include "windows.h"
#include <memory>
#include "tchar.h"

namespace console {
  struct Settings;

  #pragma warning(push)
  #pragma warning(disable : 4200)
  struct MessageData {
    size_t size;
    HWND   console_window;
    bool   adjust;
    size_t cmd_line_length;
    size_t working_directory_length;
    TCHAR  char_data[];
  };
  #pragma warning(pop)
  typedef std::unique_ptr<MessageData, void (*)(void *)> MsgDataPtr;

  class __declspec(novtable) IRootWindow {
    public:
      virtual ~IRootWindow() = 0;
      virtual bool spawn_window(const MessageData & message_data) = 0;
      virtual bool spawn_window(const Settings & settings) = 0;
      virtual HWND hwnd(void) const = 0;
  };

  typedef std::unique_ptr<IRootWindow> RootWindowPtr;
  RootWindowPtr get_root_window(HINSTANCE hInstance, const tstring & exe_dir, tstring & message);

}

#endif