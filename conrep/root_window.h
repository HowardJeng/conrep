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