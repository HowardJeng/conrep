// root_window.cpp
// implements root window class
#include "root_window.h"

#include "windows.h"

#include <map>
#include <sstream>

#include "assert.h"
#include "console_window.h"
#include "d3root.h"
#include "except_handle.h"
#include "exception.h"
#include "file_util.h"
#include "message.h"
#include "program_options.h"
#include "reg.h"
#include "settings.h"
#include "timer.h"
#include "win_util.h"
#include "window.h"

namespace console {

  class RootWindow : public IRootWindow, public Window<RootWindow> {
    public:
      RootWindow(HINSTANCE hInstance, const tstring & exe_dir, tstring & message);
      ~RootWindow() {}
      
      bool spawn_window(const MessageData & message_data);
      bool spawn_window(const Settings & settings);
        
      HWND hwnd(void) const;
    private:
      typedef std::map<HWND, WindowPtr> WindowMap;

      RootPtr         root_;
      WindowMap       window_map_;
      HINSTANCE       hInstance_;
      WallpaperInfo   wallpaper_info_;
      FILETIME        wallpaper_write_time_;

      void on_close_msg(HWND window);
      void on_lost_device(void) {
        if (root_->is_device_lost()) {
          for (WindowMap::iterator itr = window_map_.begin(); itr != window_map_.end(); ++itr) {
            itr->second->dispose_resources();
          }
          HRESULT hr = root_->try_recover();
          if (hr == D3DERR_DEVICENOTRESET) {
            // restore surfaces
            for (WindowMap::iterator itr = window_map_.begin(); itr != window_map_.end(); ++itr) {
              itr->second->restore_resources();
            }
          } else if (hr == D3DERR_DEVICELOST) {
            // something still has the device; do nothing
          } else if (FAILED(hr)) {
            DX_EXCEPT("Failure to recover device. ", hr);
          }
        }
      }
      void broadcast_message(AppMessage message,  WPARAM wParam = 0, LPARAM lParam = 0) {
        for (WindowMap::iterator itr = window_map_.begin();
              itr != window_map_.end();
              ++itr) {
          if (!PostMessage(itr->first, message, wParam, lParam)) WIN_EXCEPT("Failed call to PostMessage(). ");
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

      LRESULT actual_wnd_proc(UINT Msg, WPARAM wParam, LPARAM lParam);

      RootWindow(const RootWindow &);
      RootWindow & operator=(const RootWindow &);
  };

  RootWindow::RootWindow(HINSTANCE hInstance, const tstring & exe_dir, tstring & message)
    : Window<RootWindow>(hInstance, WS_OVERLAPPEDWINDOW, exe_dir, message, _T("Root Window")),
      hInstance_(hInstance),
      wallpaper_info_(get_wallpaper_info())
  {
    if (!wallpaper_info_.wallpaper_name.empty())
      wallpaper_write_time_ = get_modify_time(wallpaper_info_.wallpaper_name);

    ShowWindow(get_hwnd(), SW_HIDE); // no error checks as either return is legitimate
      
    try {
      root_ = get_direct3d_root(get_hwnd());
      ASSERT(root_ != nullptr);

      if (!SetTimer(get_hwnd(), TIMER_POLL_REGISTRY, POLL_TIME, 0)) WIN_EXCEPT("Failed SetTimer() call. ");
    } catch (...) {
      // if this fails reset the WndProc to DefWindowProc() as the object
      //   invariants won't hold during subsequent window messages that come
      //   along when the MessageBox() function is called.
      set_wndproc(get_hwnd(), &DefWindowProc);
      throw;
    }
  }

  bool RootWindow::spawn_window(const MessageData & message_data) {
    try {
      Settings settings(message_data.cmd_line, get_exe_dir().c_str());
      return spawn_window(settings);
    } catch (boost::program_options::error & e) {
      tstringstream sstr;
      sstr << _T("Error parsing settings: ") << e.what();
      MessageBox(NULL,  sstr.str().c_str(), _T("Error parsing settings"), MB_OK);
      return false;
    }
  }

    
  bool RootWindow::spawn_window(const Settings & settings) {
    if (!settings.run_app) return false;

    ASSERT(root_ != nullptr);
      
    WindowPtr window(create_console_window(get_hwnd(), hInstance_, settings, root_, get_exe_dir(), get_message()));
    ASSERT(window_map_.find(window->get_hwnd()) == window_map_.end());
    window_map_[window->get_hwnd()] = window;

    return true;
  }
    
  HWND RootWindow::hwnd(void) const {
    return get_hwnd();
  }

  void RootWindow::on_close_msg(HWND window) {
    WindowMap::iterator itr = window_map_.find(window);
    ASSERT(itr != window_map_.end());
    ASSERT(itr->second->get_state() == DEAD);
    window_map_.erase(itr);
    if (window_map_.empty()) {
      if (!PostMessage(get_hwnd(), WM_CLOSE, 0, 0)) WIN_EXCEPT("Failed call to PostMessage(). ");
    }
  }
    
  LRESULT RootWindow::actual_wnd_proc(UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
      case CRM_CONSOLE_CLOSE:
        on_close_msg(reinterpret_cast<HWND>(lParam));
        break;
      case CRM_LOST_DEVICE:
        on_lost_device();
        break;
      case WM_COPYDATA:
        { COPYDATASTRUCT * cbs = reinterpret_cast<COPYDATASTRUCT *>(lParam);
          MessageData * msg_data = reinterpret_cast<MessageData *>(cbs->lpData);
          if (msg_data->adjust) {
            for (WindowMap::iterator itr = window_map_.begin(); itr != window_map_.end(); ++itr) {
              if (itr->second->get_console_hwnd() == msg_data->console_window) {
                // do not use PostMessage() as the COPYDATASTRUCT will be freed when this function returns
                SendMessage(itr->second->get_hwnd(), CRM_ADJUST_WINDOW, 0, reinterpret_cast<LPARAM>(msg_data));
                return TRUE;
              }
            }
            MessageBox(NULL, _T("There doesn't seem to be an associated conrep window to adjust"), _T("--adjust error"), MB_OK);
          } else {
            spawn_window(*msg_data);
          }
          return TRUE;
        }
        break;
      case WM_TIMER:
        root_->get_color_table().poll_registry_change();
        if (!SetTimer(get_hwnd(), TIMER_POLL_REGISTRY, POLL_TIME, 0)) WIN_EXCEPT("Failed SetTimer() call. ");
        break;
      case WM_DESTROY:
        PostQuitMessage(0);
        break;
      case WM_DISPLAYCHANGE:
      case WM_SETTINGCHANGE:
        on_settingchange();
        break;
      default:
        return Window<RootWindow>::actual_wnd_proc(Msg, wParam, lParam);
    }
    return 0;
  }

  LPCTSTR RootWindow::class_name = _T("{16974a76-43bd-4129-89d5-e8ecc703c253}");
  ATOM    RootWindow::class_atom = 0;

  IRootWindow::~IRootWindow() {}
  
  RootWindowPtr get_root_window(HINSTANCE hInstance, const tstring & exe_dir, tstring & message) {
    if (!RootWindow::get_class_atom()) RootWindow::register_window_class(hInstance);
    return RootWindowPtr(new RootWindow(hInstance, exe_dir, message));
  }
}