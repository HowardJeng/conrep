// root_window.cpp
// implements root window class
#include "root_window.h"

#include "windows.h"

#include <map>
#include <sstream>

#include "assert.h"
#include "console_window.h"
#include "d3root.h"
#include "exception.h"
#include "file_util.h"
#include "message.h"
#include "reg.h"
#include "settings.h"
#include "win_util.h"

namespace console {
  class RootWindow : public IRootWindow {
    public:
      RootWindow(HINSTANCE hInstance, const tstring & exe_dir);
      ~RootWindow();
      
      static void register_window_class(HINSTANCE hInstance);
        
      bool spawn_window(LPCTSTR command_line);
      bool spawn_window(const Settings & settings);
        
      static ATOM get_class_atom(void) {
        return class_atom;
      }
        
      HWND hwnd(void) const;
    private:
      typedef std::map<HWND, WindowPtr> WindowMap;

      HWND          hWnd_;
      RootPtr       root_;
      WindowMap     window_map_;
      HINSTANCE     hInstance_;
      tstring       exe_dir_;
      WallpaperInfo wallpaper_info_;
      FILETIME      wallpaper_write_time_;
      bool          destroyed_;
        
      void on_close_msg(HWND window);
      void on_lost_device(void) {
        if (root_->is_device_lost()) {
          for (WindowMap::iterator itr = window_map_.begin();
                itr != window_map_.end();
                ++itr) {
            itr->second->dispose_resources();
          }
          HRESULT hr = root_->try_recover();
          if (hr == D3DERR_DEVICENOTRESET) {
            // restore surfaces
            for (WindowMap::iterator itr = window_map_.begin();
                  itr != window_map_.end();
                  ++itr) {
              itr->second->restore_resources();
            }
          } else if (hr == D3DERR_DEVICELOST) {
            // something still has the device; do nothing
          } else if (FAILED(hr)) {
            DX_EXCEPT("Failure to recover device. ", hr);
          }
        }
      }
      void broadcast_message(AppMessage message, 
                              WPARAM wParam = 0, 
                              LPARAM lParam = 0) {
        for (WindowMap::iterator itr = window_map_.begin();
              itr != window_map_.end();
              ++itr) {
          if (!PostMessage(itr->first, message, wParam, lParam))
            WIN_EXCEPT("Failed call to PostMessage(). ");
        }
      }
        
      void on_settingchange(void) {
        if (root_) {
          if (!root_->is_device_lost()) {
            // check if wallpaper changed; no point in doing so if the device is lost
            //   as when the device is restored the wallpaper needs to be reloaded
            //   from scratch anyways.
            WallpaperInfo current_wallpaper_info = get_wallpaper_info();
            if (wallpaper_info_ != current_wallpaper_info) {
              // different wallpaper name
              wallpaper_info_ = current_wallpaper_info;
              if (!wallpaper_info_.wallpaper_name.empty()) {
                wallpaper_write_time_ = get_modify_time(current_wallpaper_info.wallpaper_name);
              }
              root_->reset_background();
              broadcast_message(CRM_BACKGROUND_CHANGE);
            } else {
              // same wallpaper name; check to see if the modify time has
              //   changed
              if (!wallpaper_info_.wallpaper_name.empty()) {
                FILETIME ft = get_modify_time(current_wallpaper_info.wallpaper_name);
                if (CompareFileTime(&ft, &wallpaper_write_time_)) {
                  wallpaper_write_time_ = ft;
                  root_->reset_background();
                  broadcast_message(CRM_BACKGROUND_CHANGE);
                }
              }
            }
          }
          // unconditionally send a workarea change message. this can be modified
          //   to conditionally send if the workarea actually changes but the cost
          //   of the check is about as expensive as the actual message processing
          broadcast_message(CRM_WORKAREA_CHANGE);
        }
      }

      LRESULT ActualWndProc(UINT Msg, WPARAM wParam, LPARAM lParam);
      static LRESULT CALLBACK InitialWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
      static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

      static LPCTSTR class_name;
      static ATOM    class_atom; 
  };

  RootWindow::RootWindow(HINSTANCE hInstance, const tstring & exe_dir)
    : hWnd_(NULL),
      hInstance_(hInstance),
      exe_dir_(exe_dir),
      wallpaper_info_(get_wallpaper_info()),
      destroyed_(false)
  {
    ASSERT(HIWORD(class_atom) == 0);
    ASSERT(class_atom);
    if (!class_atom) 
      MISC_EXCEPT("Attempted to create window without registering class.");
    if (!wallpaper_info_.wallpaper_name.empty())
      wallpaper_write_time_ = get_modify_time(wallpaper_info_.wallpaper_name);

    HWND hWnd = CreateWindow(reinterpret_cast<LPCTSTR>(class_atom),
                              _T("Root Window"),
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              NULL, 
                              NULL, 
                              hInstance,
                              this);
    if (!hWnd) WIN_EXCEPT("Failed call to CreateWindow(). ");
    ASSERT(hWnd == hWnd_);
    ShowWindow(hWnd_, SW_HIDE); // no error checks as either return is legitimate
      
    try {
      root_ = get_direct3d_root(hWnd_);
      ASSERT(root_ != nullptr);
    } catch (...) {
      // if this fails reset the WndProc to DefWindowProc() as the object
      //   invariants won't hold during subsequent window messages that come
      //   along when the MessageBox() function is called.
      set_wndproc(hWnd_, &DefWindowProc);
      throw;
    }
  }
    
  RootWindow::~RootWindow() {
    // in case of an exception, C++ object can be potentially deleted before
    //   WM_DESTROY is processed
    if (!destroyed_) {
      set_wndproc(hWnd_, &DefWindowProc);
      DestroyWindow(hWnd_);
    }
  }

  void RootWindow::register_window_class(HINSTANCE hInstance) {
    ASSERT(!class_atom);
    if (class_atom)
      MISC_EXCEPT("RootWindow::register_window_class() called multiple times.");
    
    WNDCLASS wc = {
      CS_HREDRAW | CS_VREDRAW,
      &RootWindow::InitialWndProc,
      0,
      0,
      hInstance,
      NULL, 
      NULL, 
      reinterpret_cast<HBRUSH>(COLOR_BACKGROUND),
      NULL,
      RootWindow::class_name
    };
      
    class_atom = RegisterClass(&wc);
    if (class_atom == 0) 
      WIN_EXCEPT("Failed call to RegisterClass(). ");
  }

  bool RootWindow::spawn_window(LPCTSTR command_line) {
    try {
      Settings settings(command_line, exe_dir_.c_str());
      return spawn_window(settings);
    } catch (std::exception & e) {
      tstringstream sstr;
      sstr << _T("Error parsing settings: ") << e.what();
      MessageBox(NULL, 
                  sstr.str().c_str(),
                  _T("Error parsing settings"),
                  MB_OK);
      return false;
    }
  }

    
  bool RootWindow::spawn_window(const Settings & settings) {
    if (!settings.run_app) return false;
    try {
      ASSERT(root_ != nullptr);
      
      WindowPtr window(create_console_window(hWnd_,
                                              hInstance_,
                                              settings, 
                                              root_));
      ASSERT(window_map_.find(window->get_hwnd()) == window_map_.end());
      window_map_[window->get_hwnd()] = window;
    } catch (std::exception & e) {
      tstringstream sstr;
      sstr << _T("Error creating console window: ") << e.what();
      MessageBox(NULL, 
                  sstr.str().c_str(),
                  _T("Error creating console window"),
                  MB_OK);
      return false;
    }
    return true;
  }
    
  HWND RootWindow::hwnd(void) const {
    return hWnd_;
  }

  void RootWindow::on_close_msg(HWND window) {
    WindowMap::iterator itr = window_map_.find(window);
    ASSERT(itr != window_map_.end());
    ASSERT(itr->second->get_state() == DEAD);
    window_map_.erase(itr);
    if (window_map_.empty()) {
      if (!PostMessage(hWnd_, WM_CLOSE, 0, 0))
        WIN_EXCEPT("Failed call to PostMessage(). ");
    }
  }
    
  LRESULT RootWindow::ActualWndProc(UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
      case CRM_CONSOLE_CLOSE:
        on_close_msg(reinterpret_cast<HWND>(lParam));
        break;
      case CRM_LOST_DEVICE:
        on_lost_device();
        break;
      case WM_COPYDATA:
        { COPYDATASTRUCT * cbs = reinterpret_cast<COPYDATASTRUCT *>(lParam);
          spawn_window(reinterpret_cast<LPCTSTR>(cbs->lpData));
        }
        break;
      case WM_CREATE:
        { LPCREATESTRUCT create_struct = reinterpret_cast<LPCREATESTRUCT>(lParam);
          void * lpCreateParam = create_struct->lpCreateParams;
          RootWindow * this_window = reinterpret_cast<RootWindow *>(lpCreateParam);
          ASSERT(this_window == this);
          return DefWindowProc(hWnd_, Msg, wParam, lParam);
        }
      case WM_DESTROY:
        // Setting window proc to DefWindowProc allows the C++ object for
        //   the window to be deleted immediately.
        destroyed_ = true;
        set_wndproc(hWnd_, &DefWindowProc);
        PostQuitMessage(0);
        break;
      case WM_DISPLAYCHANGE:
      case WM_SETTINGCHANGE:
        on_settingchange();
        break;
      default:
        return DefWindowProc(hWnd_, Msg, wParam, lParam);
    }
    return 0;
  }

  LRESULT CALLBACK RootWindow::InitialWndProc(HWND hWnd,
                                              UINT Msg,
                                              WPARAM wParam,
                                              LPARAM lParam) {
    if (Msg == WM_NCCREATE) {
      LPCREATESTRUCT create_struct = reinterpret_cast<LPCREATESTRUCT>(lParam);
      void * lpCreateParam = create_struct->lpCreateParams;
      RootWindow * this_window = reinterpret_cast<RootWindow *>(lpCreateParam);
      ASSERT(this_window->hWnd_ == 0);
      this_window->hWnd_ = hWnd;
      set_userdata(hWnd, this_window);
      set_wndproc(hWnd, &RootWindow::StaticWndProc);
      return this_window->ActualWndProc(Msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
  }
    
  LRESULT CALLBACK RootWindow::StaticWndProc(HWND hWnd,
                                              UINT Msg,
                                              WPARAM wParam,
                                              LPARAM lParam) {
    LONG_PTR user_data = GetWindowLongPtr(hWnd, GWLP_USERDATA);
    RootWindow * this_window = reinterpret_cast<RootWindow *>(user_data);
    ASSERT(this_window);
    ASSERT(hWnd == this_window->hWnd_); 
    return this_window->ActualWndProc(Msg, wParam, lParam);
  }
        
  LPCTSTR RootWindow::class_name = _T("{16974a76-43bd-4129-89d5-e8ecc703c253}");
  ATOM    RootWindow::class_atom = 0;

  IRootWindow::~IRootWindow() {}
  
  std::auto_ptr<IRootWindow> get_root_window(HINSTANCE hInstance, const tstring & exe_dir) {
    if (!RootWindow::get_class_atom()) RootWindow::register_window_class(hInstance);
    std::auto_ptr<IRootWindow> tmp(new RootWindow(hInstance, exe_dir));
    return tmp;
  }
}