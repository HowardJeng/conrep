// console_window.cpp
// implementation of created console window class 
// TODO: this file is probably too big; refactor some of the functionality out

#include "console_window.h"

#include <boost/make_shared.hpp>

#include "console_util.h"
#include "context_menu.h"
#include "d3root.h"
#include "dimension_ops.h"
#include "except_handle.h"
#include "exception.h"
#include "message.h"
#include "resource.h"
#include "root_window.h"
#include "settings.h"
#include "shell_process.h"
#include "text_renderer.h"
#include "timer.h"
#include "win_util.h"

#define CLOSE_SELF() do { close_self(); return; } while (0)

namespace console {
  const DWORD WINDOW_STYLE = WS_POPUPWINDOW | WS_VSCROLL;

  class ConsoleWindowImpl : public IConsoleWindow {
    public:
      ConsoleWindowImpl(HWND hub, HINSTANCE hInstance, Settings settings, RootPtr root, const tstring & exe_dir, tstring & message)
        : hub_(hub),
          hWnd_(NULL),
          root_(root), 
          shell_process_(settings),
          active_(true),
          state_(INITIALIZING),
          maximize_(settings.maximize),
          console_visible_(false),
          scrollbar_width_(GetSystemMetrics(SM_CXVSCROLL)),
          snap_distance_(settings.snap_distance),
          device_(root->device()),
          sprite_(root->sprite()),
          white_texture_(root->white_texture()),
          menu_(get_context_menu(hInstance)),
          work_area_(get_work_area()),
          text_renderer_(root, settings),
          active_post_alpha_(static_cast<unsigned char>(settings.active_post_alpha)),
          inactive_post_alpha_(static_cast<unsigned char>(settings.inactive_post_alpha)),
          exe_dir_(exe_dir),
          message_(message)
      {
        ASSERT(settings.active_post_alpha <= std::numeric_limits<unsigned char>::max());
        ASSERT(settings.inactive_post_alpha <= std::numeric_limits<unsigned char>::max());

        // window size stuff
        Dimension max_window_dim = get_max_window_dim(work_area_);
        Dimension max_console_dim = text_renderer_.console_dim_from_window_size(max_window_dim, scrollbar_width_, WINDOW_STYLE);

        Dimension console_dim = min(Dimension(settings.columns, settings.rows), max_console_dim);
        text_renderer_.resize_buffers(console_dim);
          
        Dimension client_dim = text_renderer_.get_client_size();
        Dimension window_dim = calc_window_size(client_dim, scrollbar_width_, WINDOW_STYLE);

        ASSERT(window_dim.height <= max_window_dim.height);
        ASSERT(window_dim.width <= max_window_dim.width);

        if ((console_dim.height != settings.rows) || (console_dim.width  != settings.columns)) {
          if (ProcessLock pl = shell_process_) {
            resize_console(Dimension(console_dim), pl);
          } else {
            MISC_EXCEPT("Shell process terminated before window was created.");
          }
        }
          
        if (maximize_) window_dim = max_window_dim;
          
        // actually create the window
        ASSERT(HIWORD(class_atom) == 0);
        ASSERT(class_atom);
        HWND hWnd = CreateWindow(reinterpret_cast<LPCTSTR>(class_atom),
                                  _T("Console Window"),
                                  WINDOW_STYLE,
                                  0,
                                  0,
                                  window_dim.width,
                                  window_dim.height,
                                  NULL,
                                  NULL,
                                  hInstance,
                                  this);
        // a number of messages should already have been processed including WM_CREATE
        if (!hWnd) WIN_EXCEPT("Failed call to CreateWindow(). ");
        ASSERT(hWnd == hWnd_);
        ShowWindow(hWnd_, SW_SHOW); // no error checks as either return is legitimate
          
        try {
          set_z_order(settings.z_order);

          // Direct3D stuff
          if (maximize_) {
            client_dim = get_client_dim(max_window_dim, scrollbar_width_, WINDOW_STYLE);
          }
          swap_chain_ = root_->get_swap_chain(hWnd_, client_dim);
          HRESULT hr = swap_chain_->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &render_target_);
          if (FAILED(hr)) DX_EXCEPT("Failed IDirect3DSwapChain9::GetBackBuffer() call ", hr);
          text_renderer_.create_texture(root_, client_dim);

          // start the machinery running
          state_ = RUNNING;
          if (!SetTimer(hWnd_, TIMER_REPAINT, REPAINT_TIME, 0)) WIN_EXCEPT("Failed SetTimer() call. ");
          if (!UpdateWindow(hWnd_)) WIN_EXCEPT("Failed UpdateWindow() call. ");
        } catch (...) {
          // if this fails reset the WndProc to DefWindowProc() as the object
          //   invariants won't hold during subsequent window messages that come
          //   along when the MessageBox() function is called.
          set_wndproc(hWnd_, &DefWindowProc);
          throw;
        }
      }

      ~ConsoleWindowImpl() {
        // in case C++ object is deleted before WM_DESTROY is processed
        if (state_ != DEAD) {
          set_wndproc(hWnd_, &DefWindowProc);
          DestroyWindow(hWnd_);
        }
      }
      
      static void register_window_class(HINSTANCE hInstance) {
        ASSERT(!class_atom);
        WNDCLASS wc = {
          CS_HREDRAW | CS_VREDRAW,
          &ConsoleWindowImpl::initial_wnd_proc,
          0,
          0,
          hInstance,
          NULL,
          NULL,
          NULL,
          NULL,
          ConsoleWindowImpl::class_name
        };
        class_atom = RegisterClass(&wc);
        if (class_atom == 0) WIN_EXCEPT("Failed call to RegisterClass(). ");
      }
        
      HWND get_hwnd(void) const {
        return hWnd_;
      }

      HWND get_console_hwnd(void) const {
        return shell_process_.window_handle();
      }

      WindowState get_state(void) const {
        return state_;
      }
        
      static ATOM get_class_atom(void) {
        return class_atom;
      }

      void dispose_resources(void) {
        // release handles to shared resources
        sprite_ = 0;
        white_texture_ = 0;
        // free per window resources
        swap_chain_ = 0;
        render_target_ = 0;
        text_renderer_.dispose();
      }

      void restore_resources(void) {
        sprite_ = root_->sprite();
        white_texture_ = root_->white_texture();

        text_renderer_.recreate_font(device_);

        Dimension client_dim = text_renderer_.get_client_size();
        if (maximize_) {
          Dimension max_window_dim = get_max_window_dim(work_area_);
          client_dim = get_client_dim(max_window_dim, scrollbar_width_, WINDOW_STYLE);
        }

        swap_chain_ = root_->get_swap_chain(hWnd_, client_dim);
        HRESULT hr = swap_chain_->GetBackBuffer(0,  D3DBACKBUFFER_TYPE_MONO,  &render_target_);
        if (FAILED(hr)) DX_EXCEPT("Failed IDirect3DSwapChain9::GetBackBuffer() call ", hr);
        text_renderer_.create_texture(root_, client_dim);
      }
    private:
      HWND hub_;  // handle to controller window
      HWND hWnd_; // self handle
      RootPtr root_; // pointer to per application Direct3D information
      ShellProcess shell_process_; // interface to spawned process

      bool active_; // if window has focus
      WindowState state_;
        
      bool maximize_;        // if the window was created to cover work area
      bool console_visible_; // if the console associated with the shell process is visible
        
      INT scrollbar_width_;
      int snap_distance_; // distance to edges of work area before window adjustment
        
      // pointers to shared direct3d objects
      DevicePtr  device_;
      SpritePtr  sprite_;
      TexturePtr white_texture_;

      // pointers to per window direct3d objects
      SwapChainPtr swap_chain_;
      SurfacePtr   render_target_;

      MenuPtr menu_;
      RECT work_area_;
      ZOrder z_order_;
      TextRenderer text_renderer_;

      unsigned char active_post_alpha_;
      unsigned char inactive_post_alpha_;

      const tstring & exe_dir_;
      tstring & message_;
    private:
      void resize_console(Dimension console_dim, ProcessLock & pl) {
        text_renderer_.resize_buffers(pl.resize(console_dim));
      }

      BOOL on_create(void) {
        ASSERT(state_ == INITIALIZING);

        if (!SetForegroundWindow(hWnd_)) {
          DWORD err = GetLastError();
          if (err != 0) WIN_EXCEPT2("Failed call to SetForegroundWindow(). ", err);
        }
          
        return TRUE;
      }
        
      BOOL draw_background_enum_proc_impl(HMONITOR hMonitor, HDC, LPRECT lprcMonitor) {
        POINT p = { 0, 0 };
        if (!ClientToScreen(hWnd_, &p)) WIN_EXCEPT("Failed call to ClientToScreen(). ");
        p.x -= lprcMonitor->left;
        p.y -= lprcMonitor->top;

        D3DXVECTOR3 translation(static_cast<float>(-p.x), static_cast<float>(-p.y), 0.0f);
        HRESULT hr = sprite_->Draw(root_->background_texture(hMonitor), 0, 0, &translation, 0xffffffff);
        if (FAILED(hr)) DX_EXCEPT("Failed call to ID3DXSprite::Draw(). ", hr);
        return TRUE;
      }
        
      static BOOL CALLBACK draw_background_enum_proc(HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData) {
        ConsoleWindowImpl * wnd = reinterpret_cast<ConsoleWindowImpl *>(dwData);
        return wnd->draw_background_enum_proc_impl(hMonitor, hdc, lprcMonitor);
      }
        
      void on_paint(void) {
        if (state_ == RUNNING) {
          if (root_->is_device_lost()) {
            PostMessage(hub_, CRM_LOST_DEVICE, 0, 0);
          } else {
            root_->set_render_target(render_target_);

            {
              SceneLock scene(*root_);
              EnumDisplayMonitors(NULL, NULL, &draw_background_enum_proc, reinterpret_cast<LPARAM>(this));

              if (active_) {
                text_renderer_.render(sprite_, D3DCOLOR_ARGB(active_post_alpha_, 0xff, 0xff, 0xff));
                // if GetTickCount() rolls over it doesn't matter
                #pragma warning(suppress: 28159)
                if ((GetTickCount() / 500) % 2) text_renderer_.draw_cursor(sprite_);
              } else {
                text_renderer_.render(sprite_, D3DCOLOR_ARGB(inactive_post_alpha_, 0xff, 0xff, 0xff));
              }
            }

            HRESULT hr = swap_chain_->Present(0, 0, 0, 0, 0);
            if (FAILED(hr)) {
              if (hr == D3DERR_DEVICELOST) {
                root_->set_device_lost();
              } else {
                DX_EXCEPT("Failed call to IDirect3DSwapChain9::Present(). ", hr);
              }
            } else {
              if (!ValidateRect(hWnd_, NULL)) WIN_EXCEPT("Failed call to ValidateRect(). ");
            }
          }
        }
      }
        
      void update_text_buffer(ProcessLock & pl) {
        ASSERT(pl == true);
        ASSERT(shell_process_.attached());
        if ((state_ == RUNNING) && (!root_->is_device_lost())) {
          text_renderer_.update_text_buffer(pl, root_, sprite_, active_);
          if (!SetTimer(hWnd_, TIMER_REPAINT, REPAINT_TIME, 0)) WIN_EXCEPT("Failed call to SetTimer(). ");
        }
      }
        
      void update_scrollbar(void) {
        if (state_ == RUNNING) {
          SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
          if (!GetScrollInfo(shell_process_.window_handle(), SB_VERT, &si)) {
            DWORD err = GetLastError();
            if (err == ERROR_INVALID_WINDOW_HANDLE) {
              // this can happen if update_scrollbar is called after the shell process
              //   terminates but before the timer detects it.
              CLOSE_SELF();
            }
            WIN_EXCEPT2("Failed call to GetScrollInfo(). ", err);
          }
          ASSERT(si.cbSize == sizeof(SCROLLINFO));
          ASSERT(si.fMask == SIF_ALL);
          // SetScrollInfo()'s return doesn't contain an error value so can be ignored
          SetScrollInfo(hWnd_, SB_VERT, &si, TRUE);
        }
      }
        
      void close_self(void) {
        state_ = CLOSING;
        dispose_resources();
        if (!PostMessage(hWnd_, WM_CLOSE, 0, 0)) WIN_EXCEPT("Failed call to PostMessage(). ");
      }
        
      void update_console_size(ProcessLock & pl) {
        ASSERT(state_ == RUNNING);
        if (text_renderer_.poll_console_size(pl)) {
          //resize window
          if (!maximize_) {
            // if the window is maximized, then the only thing we can do is make
            //   sure the console buffers are big enough
            // otherwise change the actual window size
            state_ = RESETTING;
            Dimension new_client_dim = text_renderer_.get_client_size();
            Dimension new_window_dim = calc_window_size(new_client_dim, scrollbar_width_, WINDOW_STYLE);
            resize_window(new_client_dim, new_window_dim);
            state_ = RUNNING;
          }
        }
      }
        
      void on_timer(void) {
        ASSERT(state_ == RUNNING);
        if (check_active_changed()) {
          text_renderer_.invalidate();
        }
        if (ProcessLock pl = shell_process_) {
          update_console_size(pl);
          update_scrollbar();
          set_window_title(pl);
          update_text_buffer(pl);
        } else {
          CLOSE_SELF();
        }
        invalidate_self();
      }
        
      void set_window_title(const ProcessLock &) {
        ASSERT(shell_process_.attached());
        const int BUFFER_SIZE = 0x800;
        
        TCHAR console_title[BUFFER_SIZE] = {};
        if (!GetConsoleTitle(console_title, BUFFER_SIZE)) WIN_EXCEPT("Failed call to GetConsoleTitle(). ");
        
        // Profiler indicates that SetWindowText() is sufficiently slower than GetWindowtext() that checking if
        //   the text is the same first makes sense
        TCHAR window_text[BUFFER_SIZE] = {};
        if (!GetWindowText(hWnd_, window_text, BUFFER_SIZE)) WIN_EXCEPT("Failed call to GetWindowText(). ");
        if (_tcsncmp(console_title, window_text, BUFFER_SIZE) == 0) return;

        if (!SetWindowText(hWnd_, console_title)) WIN_EXCEPT("Failed call to SetWindowText(). ");
      }

      BOOL on_moving(LPARAM lParam) { 
        RECT * r = reinterpret_cast<RECT *>(lParam);
        if (maximize_) {
          r->bottom -= r->top;
          r->right  -= r->left;
          r->top    = 0;
          r->left   = 0;
        } else {
          HMONITOR mon = MonitorFromWindow(hWnd_, MONITOR_DEFAULTTOPRIMARY);
          MONITORINFO info = { sizeof(MONITORINFO) };
          if (!GetMonitorInfo(mon, &info)) WIN_EXCEPT("Failed call to GetMonitorInfo(). ");
          work_area_ = info.rcWork;
          snap_window(*r, info.rcWork, snap_distance_);
        }
        return TRUE;
      }
        
      void on_move(LPARAM) { 
        invalidate_self();
      }

      bool check_active_changed(void) {
        bool is_active = (GetForegroundWindow() == hWnd_);
        if (is_active != active_) {
          active_ = is_active;
          return true;
        }
        return false;
      }

      void on_activate(void) {
        if (check_active_changed()) {
          text_renderer_.invalidate();
          invalidate_self();
        }
      }
        
      void move_window(int x, int y) {
        if (!SetWindowPos(hWnd_, 
                          0, // ignored
                          x,
                          y,
                          0, // ignored
                          0, // ignored
                          SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER))
          WIN_EXCEPT("Failed call to SetWindowPos().");
      }
        
      void on_workarea_change(void) {
        ASSERT(state_ == RUNNING);
        int new_scrollbar_width = GetSystemMetrics(SM_CXVSCROLL);
            
        if (maximize_) {
          SendMessage(shell_process_.window_handle(), WM_SETTINGCHANGE, 0, 0);
          state_ = RESETTING;
          
          Dimension old_window_dim = get_max_window_dim(work_area_);

          HMONITOR mon = MonitorFromWindow(hWnd_, MONITOR_DEFAULTTOPRIMARY);
          MONITORINFO info = { sizeof(MONITORINFO) };
          if (!GetMonitorInfo(mon, &info)) WIN_EXCEPT("Error in GetMonitorInfo() call. ");

          Dimension new_window_dim = get_max_window_dim(info.rcWork);
          work_area_ = info.rcWork;
          POINT p = { work_area_.left, work_area_.top };
          
          if ((old_window_dim != new_window_dim) || (new_scrollbar_width != scrollbar_width_)) {
            scrollbar_width_ = new_scrollbar_width;
            resize_window(get_client_dim(new_window_dim, scrollbar_width_, WINDOW_STYLE), new_window_dim);
          }
          Dimension console_dim = text_renderer_.console_dim_from_window_size(new_window_dim,
                                                                              scrollbar_width_,
                                                                              WINDOW_STYLE);
          move_window(p.x, p.y);
          if (ProcessLock pl = shell_process_) {
            resize_console(console_dim, pl);
            state_ = RUNNING;
            update_text_buffer(pl);
          } else {
            CLOSE_SELF();
          }
        } else {
          int delta_scrollbar_width = new_scrollbar_width - scrollbar_width_;
          if (new_scrollbar_width != scrollbar_width_) {
            scrollbar_width_ = new_scrollbar_width;
            Dimension new_client_dim = text_renderer_.get_client_size();
            Dimension new_window_dim = calc_window_size(new_client_dim, scrollbar_width_,WINDOW_STYLE);
            state_ = RESETTING;
            resize_window(new_client_dim, new_window_dim);
            state_ = RUNNING;             
          }
          
          RECT r;
          if (!GetWindowRect(hWnd_, &r)) WIN_EXCEPT("Failed call to GetWindowRect().");

          int delta_left   = r.left   - work_area_.left;
          int delta_right  = r.right  - work_area_.right;
          int delta_top    = r.top    - work_area_.top;
          int delta_bottom = r.bottom - work_area_.bottom;
            
          int new_x = r.left;
          int new_y = r.top;

          HMONITOR mon = MonitorFromWindow(hWnd_, MONITOR_DEFAULTTOPRIMARY);
          MONITORINFO info = { sizeof(MONITORINFO) };
          if (!GetMonitorInfo(mon, &info)) WIN_EXCEPT("Failed call to GetMonitorInfo(). ");
           
          if (delta_left == 0)   new_x  = info.rcWork.left;
          if (delta_top  == 0)   new_y  = info.rcWork.top;
          if (delta_right == 0)  {
            new_x += info.rcWork.right - work_area_.right;
            new_x -= delta_scrollbar_width;
          }
          if (delta_bottom == 0) new_y += info.rcWork.bottom - work_area_.bottom;

          work_area_ = info.rcWork;
          move_window(new_x, new_y);
        }
      }
        
      void toggle_console_visible(void) {
        console_visible_ = !console_visible_;
        // ShowWindow()'s return value doesn't contain error information so can be ignored
        if (console_visible_) {
          ShowWindow(shell_process_.window_handle(), SW_SHOW);
        } else {
          ShowWindow(shell_process_.window_handle(), SW_HIDE);
        }
      }
        
      void change_font(void) {
        ASSERT(state_ == RUNNING);
        if (text_renderer_.choose_font(device_, hWnd_)) {
          state_ = RESETTING;
          if (maximize_) {
            Dimension window_dim = get_max_window_dim(work_area_);
            Dimension console_dim = text_renderer_.console_dim_from_window_size(window_dim, scrollbar_width_, WINDOW_STYLE);

            if (ProcessLock pl = shell_process_) {
              resize_console(console_dim, pl);
            } else {
              CLOSE_SELF();
            }
          } else {
            Dimension new_client_dim = text_renderer_.get_client_size();
            Dimension new_window_dim = calc_window_size(new_client_dim, scrollbar_width_, WINDOW_STYLE);
            resize_window(new_client_dim, new_window_dim);
          }
          state_ = RUNNING;
          if (ProcessLock pl = shell_process_) {
            update_text_buffer(pl);
          } else {
            CLOSE_SELF();
          }
        }
      }

      void resize_window(Dimension new_client_dim, Dimension new_window_dim) {
        ASSERT(state_ == RESETTING);
        if (!SetWindowPos(hWnd_, 0, 0, 0, new_window_dim.width, new_window_dim.height, SWP_NOMOVE | SWP_NOZORDER))
          WIN_EXCEPT("Failed call to SetWindowPos().");

        if (!root_->is_device_lost()) {
          // reset Direct3D interfaces
          HRESULT hr = root_->device()->TestCooperativeLevel();
          if (FAILED(hr)) {
            if (hr == D3DERR_DEVICELOST) {
              root_->set_device_lost();
            } else {
              DX_EXCEPT("Failed call to IDirect3DDevice9::TestCooperativeLevel().", hr);
            }
          } else {
            swap_chain_ = root_->get_swap_chain(hWnd_, new_client_dim);
            render_target_ = 0;
            hr = swap_chain_->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &render_target_);
            if (FAILED(hr)) DX_EXCEPT("Failed call to IDirect3DSwapChain9::GetBackBuffer().", hr);
            text_renderer_.create_texture(root_, new_client_dim);
          }
        }
      }
        
      void on_command(int id) {
        switch (id) {
          case ID_CHANGEFONT:
            change_font();
            break;
          case ID_EXIT:
            close_self();
            if (!PostMessage(shell_process_.window_handle(), WM_CLOSE, 0, 0))
              WIN_EXCEPT("Failed call to PostMessage().");
            break;
          case ID_SHOWCONSOLE:
            toggle_console_visible();
            break;
          case ID_SHOWEXTENDEDCHARACTERS:
            text_renderer_.toggle_extended_chars();
            break;
          case ID_ALWAYSONTOP:
            if (z_order_ == Z_TOP) {
              set_z_order(Z_NORMAL);
            } else {
              set_z_order(Z_TOP);
            }
            break;
          case ID_ALWAYSONBOTTOM:
            if (z_order_ == Z_BOTTOM) {
              set_z_order(Z_NORMAL);
            } else {
              set_z_order(Z_BOTTOM);
            }
            break;
        }
      }
        
      void invalidate_self(void) {
        if (!InvalidateRect(hWnd_, 0, FALSE)) WIN_EXCEPT("Failed call to InvalidateRect(). ");
      }

      void set_z_order(ZOrder z_order) {
        z_order_ = z_order;
        switch (z_order) {
          case Z_BOTTOM:
            set_z_bottom(hWnd_);
            return;
          case Z_NORMAL:
            set_z_normal(hWnd_);
            return;
          case Z_TOP:
            set_z_top(hWnd_);
            return;
          default:
            ASSERT(false);
        }
      }

      void on_adjust(MessageData * msg_data) {
        Settings settings(msg_data->cmd_line);

        if (settings.scl_z_order) set_z_order(settings.z_order);
        if (settings.scl_snap_distance) snap_distance_ = settings.snap_distance;
        if (settings.scl_active_post_alpha) {
          ASSERT(settings.active_post_alpha <= std::numeric_limits<unsigned char>::max());
          active_post_alpha_ = static_cast<unsigned char>(settings.active_post_alpha);
        }
        if (settings.scl_inactive_post_alpha) {
          ASSERT(settings.inactive_post_alpha <= std::numeric_limits<unsigned char>::max());
          inactive_post_alpha_ = static_cast<unsigned char>(settings.inactive_post_alpha);
        }

        state_ = RESETTING;
        if (settings.scl_maximize) {
          maximize_ = settings.maximize;
          if (maximize_) {
            HMONITOR mon = MonitorFromWindow(hWnd_, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO info = { sizeof(MONITORINFO) };
            if (!GetMonitorInfo(mon, &info)) WIN_EXCEPT("Error in GetMonitorInfo() call. ");

            Dimension new_window_dim = get_max_window_dim(info.rcWork);
            work_area_ = info.rcWork;
            POINT p = { work_area_.left, work_area_.top };
          
            resize_window(get_client_dim(new_window_dim, scrollbar_width_, WINDOW_STYLE), new_window_dim);
            move_window(p.x, p.y);
          }
        }

        text_renderer_.adjust(device_, settings);
        if (maximize_) {
          Dimension window_dim = get_max_window_dim(work_area_);
          Dimension console_dim = text_renderer_.console_dim_from_window_size(window_dim, scrollbar_width_, WINDOW_STYLE);

          if (ProcessLock pl = shell_process_) {
            resize_console(console_dim, pl);
          } else {
            CLOSE_SELF();
          }
        } else {
          if (settings.scl_maximize || settings.scl_columns || settings.scl_rows) {
            if (ProcessLock pl = shell_process_) {
              resize_console(Dimension(settings.columns, settings.rows), pl);
            } else {
              CLOSE_SELF();
            }
          }

          Dimension new_client_dim = text_renderer_.get_client_size();
          Dimension new_window_dim = calc_window_size(new_client_dim, scrollbar_width_, WINDOW_STYLE);
          resize_window(new_client_dim, new_window_dim);
        }
        state_ = RUNNING;
      }

      LRESULT actual_wnd_proc(UINT Msg, WPARAM wParam, LPARAM lParam) {
        switch (Msg) {
          case CRM_BACKGROUND_CHANGE:
            invalidate_self();
            break;
          case CRM_WORKAREA_CHANGE:
            on_workarea_change();
            break;
          case CRM_ADJUST_WINDOW:
            { MessageData * msg_data = reinterpret_cast<MessageData *>(lParam);
              ASSERT(msg_data);
              on_adjust(msg_data);
            }
            break;
          case WM_ACTIVATE:
            on_activate();
            return 0;
          case WM_COMMAND:
            on_command(LOWORD(wParam));
            return 0;
          case WM_CREATE:
            { LPCREATESTRUCT create_struct = reinterpret_cast<LPCREATESTRUCT>(lParam);
              ASSERT(create_struct != nullptr);
              void * lpCreateParam = create_struct->lpCreateParams;
              ConsoleWindowImpl * this_window = reinterpret_cast<ConsoleWindowImpl *>(lpCreateParam);
              ASSERT(this_window == this);
              return on_create();
            }
          case WM_DESTROY:
            state_ = DEAD;
            // Setting window proc to DefWindowProc allows the C++ object for
            //   the window to be deleted immediately.
            set_wndproc(hWnd_, &DefWindowProc);
            SendMessage(hub_, CRM_CONSOLE_CLOSE, 0, reinterpret_cast<LPARAM>(hWnd_));
            break;
          case WM_MOVE:
            on_move(lParam);
            return 0;
          case WM_MOVING:
            return on_moving(lParam);
          case WM_NCHITTEST:
            { POINTS p = MAKEPOINTS(lParam); 
              WINDOWINFO wi = {};
              if (!GetWindowInfo(hWnd_, &wi)) WIN_EXCEPT("Failed call to GetWindowInfo(). ");
              int window_x = p.x - wi.rcWindow.left;
              int scroll_start = wi.rcWindow.right - wi.rcWindow.left + 1
                                - 2 * wi.cxWindowBorders
                                - scrollbar_width_;
              if (window_x >= scroll_start) return HTVSCROLL;
            }
            return HTCAPTION;
          case WM_PAINT:
            on_paint();
            break;
          case WM_QUERYDRAGICON:
            // DefWindowProc will actually cause an access violation
            //   for WM_QUERYDRAGICON if the message is passed right after
            //   the window is created.
            return NULL;
          case WM_NCRBUTTONUP:
            { POINTS p = MAKEPOINTS(lParam);
              menu_->set_console_visible(console_visible_);
              menu_->set_z_order(z_order_);
              text_renderer_.set_menu_options(menu_);
              menu_->display(hWnd_, p);
            }
            break;
          case WM_TIMER:
            on_timer();
            break;
          case WM_WINDOWPOSCHANGING:
            { WINDOWPOS * wp = reinterpret_cast<WINDOWPOS *>(lParam);
              if (z_order_ == Z_BOTTOM) {
                wp->hwndInsertAfter = HWND_BOTTOM;
              }
            }
            break;
          case WM_KEYDOWN:
          case WM_INPUTLANGCHANGEREQUEST:
          case WM_KEYUP:
          case WM_MOUSEWHEEL:
          case WM_VSCROLL:
          case WM_SYSKEYDOWN:
          case WM_SYSKEYUP:
            PostMessage(shell_process_.window_handle(), Msg, wParam, lParam);
            update_scrollbar();
            break;
          default:
            return DefWindowProc(hWnd_, Msg, wParam, lParam);
        }
        return 0;
      }

      static LRESULT CALLBACK initial_wnd_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
        if (Msg == WM_NCCREATE) {
          LPCREATESTRUCT create_struct = reinterpret_cast<LPCREATESTRUCT>(lParam);
          ASSERT(create_struct != nullptr);
          void * lpCreateParam = create_struct->lpCreateParams;
          ConsoleWindowImpl * this_window = reinterpret_cast<ConsoleWindowImpl *>(lpCreateParam);
          ASSERT(this_window != nullptr);
          ASSERT(this_window->hWnd_ == 0);
          this_window->hWnd_ = hWnd;
          set_userdata(hWnd, this_window);
          set_wndproc(hWnd, &ConsoleWindowImpl::static_wnd_proc);
          return this_window->actual_wnd_proc(Msg, wParam, lParam);
        }
        return DefWindowProc(hWnd, Msg, wParam, lParam);
      }
        
      static LRESULT CALLBACK static_wnd_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
        LONG_PTR user_data = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        ConsoleWindowImpl * this_window = reinterpret_cast<ConsoleWindowImpl *>(user_data);
        // this assertion may just be swallowed by the kernal
        ASSERT(this_window);
        __try {
          ASSERT(hWnd == this_window->hWnd_); 
          return this_window->actual_wnd_proc(Msg, wParam, lParam);
        // exception_flter() doesn't handle stack overflow properly, so just skip the
        //   the function and dump to the OS exception handler in that case.
        } __except( (GetExceptionCode() != EXCEPTION_STACK_OVERFLOW)
                      ? exception_filter(this_window->exe_dir_, *GetExceptionInformation(), this_window->message_)
                      : EXCEPTION_CONTINUE_SEARCH
                   ) {
          PostQuitMessage(EXIT_ABNORMAL_TERMINATION);
          return DefWindowProc(hWnd, Msg, wParam, lParam);
        }
      }
        
      static LPCTSTR class_name;
      static ATOM    class_atom; 
  };

  LPCTSTR ConsoleWindowImpl::class_name = _T("{58ca8e3c-c7b0-4e84-bce6-d8502b2a4a8a}");
  ATOM    ConsoleWindowImpl::class_atom = 0;
  
  IConsoleWindow::~IConsoleWindow() {}

  WindowPtr create_console_window(HWND hub, 
                                  HINSTANCE hInstance, 
                                  const Settings & settings, 
                                  RootPtr root, 
                                  const tstring & exe_dir,
                                  tstring & message) {
    if (!ConsoleWindowImpl::get_class_atom()) ConsoleWindowImpl::register_window_class(hInstance);
    return boost::make_shared<ConsoleWindowImpl>(hub, hInstance, settings, root, exe_dir, message);
  }
}