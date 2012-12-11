#include "context_menu.h"

#include "assert.h"
#include "exception.h"
#include "resource.h"

namespace console {
  class Menu : public IContextMenu {
    public:
      Menu(HINSTANCE hInstance)
        : menu_(LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1))),
          sub_(GetSubMenu(menu_, 0))
      { 
        ASSERT(menu_);
        ASSERT(sub_);
      }

      ~Menu() {
        DestroyMenu(menu_);
      }
        
      void set_console_visible(bool console_visible) {
        if (console_visible) {
          modify_menu_string(ID_SHOWCONSOLE, TEXT("Hide Console"));
        } else {
          modify_menu_string(ID_SHOWCONSOLE, TEXT("Show Console"));
        }
      }
        
      void set_extended_chars(bool extended_chars) {
        if (extended_chars) {
          modify_menu_string(ID_SHOWEXTENDEDCHARACTERS, TEXT("Hide Extended Characters"));
        } else {
          modify_menu_string(ID_SHOWEXTENDEDCHARACTERS, TEXT("Show Extended Characters"));
        }
      }
        
      void set_z_order(ZOrder z_order) {
        switch (z_order) {
          case Z_TOP:
            if (CheckMenuItem(sub_, ID_ALWAYSONTOP, MF_CHECKED) == -1)
              WIN_EXCEPT("Error in CheckMenuItem(). ");
            if (CheckMenuItem(sub_, ID_ALWAYSONBOTTOM, MF_UNCHECKED) == -1)
              WIN_EXCEPT("Error in CheckMenuItem(). ");
            break;
          case Z_BOTTOM:
            if (CheckMenuItem(sub_, ID_ALWAYSONTOP, MF_UNCHECKED) == -1)
              WIN_EXCEPT("Error in CheckMenuItem(). ");
            if (CheckMenuItem(sub_, ID_ALWAYSONBOTTOM, MF_CHECKED) == -1)
              WIN_EXCEPT("Error in CheckMenuItem(). ");
            break;
          default:
            if (CheckMenuItem(sub_, ID_ALWAYSONTOP, MF_UNCHECKED) == -1)
              WIN_EXCEPT("Error in CheckMenuItem(). ");
            if (CheckMenuItem(sub_, ID_ALWAYSONBOTTOM, MF_UNCHECKED) == -1)
              WIN_EXCEPT("Error in CheckMenuItem(). ");
        }
      }
        
      void display(HWND hWnd, const POINTS & p) {
        if (!TrackPopupMenu(sub_, 0, p.x, p.y, 0, hWnd, 0))
          WIN_EXCEPT("Error displaying popup menu. ");
      }
    private:
      HMENU menu_;
      HMENU sub_;
        
      Menu(const Menu &);
      Menu & operator=(const Menu &);

      void modify_menu_string(UINT command, LPCTSTR text) {
        if (ModifyMenu(sub_, command, MF_BYCOMMAND | MF_STRING, command, text) == 0)
          WIN_EXCEPT("Error in ModifyMenu(). ");
      }
  };
  
  MenuPtr get_context_menu(HINSTANCE hInstance) {
    return MenuPtr(new Menu(hInstance));
  }
  
  IContextMenu::~IContextMenu() {}
}