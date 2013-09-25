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

#ifndef CONREP_WINDOW_H
#define CONREP_WINDOW_H

#include "exception.h"
#include "tchar.h"
#include "win_util.h"
#include "windows.h"

namespace console {
  template <typename T>
  class Window {
    protected:
      Window(HINSTANCE hInstance, 
             DWORD window_style, 
             const tstring & exe_dir, 
             tstring & message, 
             const tstring & window_title)
       : hWnd_(NULL),
         exe_dir_(exe_dir), 
         message_(message) 
      {
        ASSERT(HIWORD(class_atom) == 0);
        ASSERT(class_atom);
        if (!class_atom) MISC_EXCEPT("Attempted to create window without registering class.");

        HWND hWnd = CreateWindow(reinterpret_cast<LPCTSTR>(class_atom),
                                  window_title.c_str(),
                                  window_style,
                                  0,
                                  0,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  NULL, 
                                  NULL, 
                                  hInstance,
                                  this);
        if (!hWnd) WIN_EXCEPT("Failed call to CreateWindow(). ");
        ASSERT(hWnd == hWnd_);
      }

      virtual ~Window() {
        set_wndproc(get_hwnd(), &DefWindowProc, true);
        DestroyWindow(get_hwnd());
      }

               HWND   get_hwnd(void)    const { return hWnd_; }
      const tstring & get_exe_dir(void) const { return exe_dir_; }
            tstring & get_message(void) const { return message_; }

      virtual LRESULT actual_wnd_proc(UINT Msg, WPARAM wParam, LPARAM lParam) {
        switch (Msg) {
          case WM_CREATE:
            { LPCREATESTRUCT create_struct = reinterpret_cast<LPCREATESTRUCT>(lParam);
              ASSERT(create_struct != nullptr);
              void * lpCreateParam = create_struct->lpCreateParams;
              Window * this_window = reinterpret_cast<Window *>(lpCreateParam);
              ASSERT(this_window == this);
            }
            break;
          case WM_DESTROY:
            // Setting window proc to DefWindowProc allows the C++ object for
            //   the window to be deleted immediately.
            set_wndproc(hWnd_, &DefWindowProc, true);
            break;
        }
        return DefWindowProc(hWnd_, Msg, wParam, lParam);
      }
    public:
      static ATOM get_class_atom(void) {
        return class_atom;
      }

      static void register_window_class(HINSTANCE hInstance) {
        ASSERT(!class_atom);
        if (class_atom) MISC_EXCEPT("Window<T>::register_window_class() called multiple times.");
    
        WNDCLASS wc = {
          CS_HREDRAW | CS_VREDRAW,
          &initial_wnd_proc,
          0,
          0,
          hInstance,
          NULL, 
          NULL, 
          reinterpret_cast<HBRUSH>(COLOR_BACKGROUND),
          NULL,
          class_name
        };
      
        class_atom = RegisterClass(&wc);
        if (class_atom == 0) WIN_EXCEPT("Failed call to RegisterClass(). ");
      }
    private:
      HWND            hWnd_;
      const tstring & exe_dir_;
      tstring &       message_;

      static LRESULT CALLBACK initial_wnd_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
        if (Msg == WM_NCCREATE) {
          LPCREATESTRUCT create_struct = reinterpret_cast<LPCREATESTRUCT>(lParam);
          ASSERT(create_struct != nullptr);
          void * lpCreateParam = create_struct->lpCreateParams;
          Window * this_window = reinterpret_cast<Window *>(lpCreateParam);
          ASSERT(this_window != nullptr);
          ASSERT(this_window->hWnd_ == 0);
          this_window->hWnd_ = hWnd;
          set_userdata(hWnd, this_window);
          set_wndproc(hWnd, &static_wnd_proc);
          return this_window->actual_wnd_proc(Msg, wParam, lParam);
        }
        return DefWindowProc(hWnd, Msg, wParam, lParam);
      }

      static LRESULT CALLBACK static_wnd_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
        LONG_PTR user_data = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        Window * this_window = reinterpret_cast<Window *>(user_data);
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

      Window(const Window &);
      Window & operator=(const Window &);
  };
}

#endif