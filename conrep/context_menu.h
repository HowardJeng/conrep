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

// context menu for the console window

#ifndef CONREP_CONTEXT_MENU_H
#define CONREP_CONTEXT_MENU_H

#include "settings.h"
#include <memory>

namespace console {
  struct __declspec(novtable) IContextMenu {
    virtual void set_console_visible(bool console_visible) = 0;
    virtual void set_extended_chars(bool extended_chars) = 0;
    virtual void set_z_order(ZOrder z_order) = 0;
    virtual void display(HWND hWnd, const POINTS & p) = 0;

    virtual ~IContextMenu() = 0;
  };
    
  typedef std::unique_ptr<IContextMenu> MenuPtr;
  MenuPtr get_context_menu(HINSTANCE hInstance);
}

#endif