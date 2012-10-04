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
    
  typedef std::auto_ptr<IContextMenu> MenuPtr;
  MenuPtr get_context_menu(HINSTANCE hInstance);
}

#endif