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

// main.cpp
// contains the entry point and initialization for application

#include "windows.h"
#include <crtdbg.h>
#include <dbghelp.h>

#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>

#include "assert.h"
#include "atl.h"
#include "except_handle.h"
#include "exception.h"
#include "file_util.h"
#include "gdiplus.h"
#include "root_window.h"
#include "settings.h"

using namespace ATL;
using namespace Gdiplus;
using namespace console;

// disables silent swalling on SEH exceptions in callbacks
void disable_process_callback_filter(void) {
  typedef BOOL (WINAPI *GetPolicyPtr)(LPDWORD lpFlags); 
  typedef BOOL (WINAPI *SetPolicyPtr)(DWORD dwFlags); 
  const DWORD PROCESS_CALLBACK_FILTER_ENABLED = 0x1;

  HMODULE kernel32 = LoadLibrary(_T("kernel32.dll"));
  if (kernel32) {
    GetPolicyPtr get_policy = (GetPolicyPtr)GetProcAddress(kernel32, "GetProcessUserModeExceptionPolicy"); 
    if (get_policy) {
      SetPolicyPtr set_policy = (SetPolicyPtr)GetProcAddress(kernel32, "SetProcessUserModeExceptionPolicy"); 
      if (set_policy) {
        DWORD flags; 
        if (get_policy(&flags)) { 
          set_policy(flags & ~PROCESS_CALLBACK_FILTER_ENABLED);
        }
      }
    }
    FreeLibrary(kernel32);
  }
}

// RAII initializer for GDI+
class GDIPlusInit { 
  public:
    GDIPlusInit() {
      GdiplusStartupInput startup;
      Status ret_val = GdiplusStartup(&token_, &startup, NULL);
      if (ret_val != Ok) GDIPLUS_EXCEPT("Failure in GdiplisStartup() call.", ret_val);
    }
    ~GDIPlusInit() {
      GdiplusShutdown(token_);
    }
  private:
    GDIPlusInit(const GDIPlusInit &);
    GDIPlusInit & operator=(const GDIPlusInit &);

    ULONG_PTR token_;
};

class SymInit {
  public:
    SymInit(void) : process_(GetCurrentProcess()) {
      SymInitialize(process_, 0, TRUE);

      DWORD options = SymGetOptions();
      options |= SYMOPT_LOAD_LINES;
      SymSetOptions(options);
    }
    ~SymInit() {
      SymCleanup(process_);
    }
  private:
    HANDLE process_;
    SymInit(const SymInit &);
    SymInit & operator=(const SymInit &);
};

class COMInit {
  public:
    COMInit(void) {
      HRESULT hr = CoInitialize(0);
      if (FAILED(hr))  DX_EXCEPT("Failed to initialize COM. ", hr);
    }
    ~COMInit() {
      CoUninitialize();
    }
  private:
    COMInit(const COMInit &);
    COMInit & operator=(const COMInit &);
};

const TCHAR APP_MUTEX_NAME[]  = _T("conrep{7c1123af-2ffe-41e7-aebd-da66f803aca7}");
const TCHAR MMAP_NAME[]       = _T("conrep{4b581c5e-5f5a-4fb7-b185-1252cea83d92}");
const TCHAR MMAP_MUTEX_NAME[] = _T("conrep{2ba41d85-95c8-46b6-82a6-0d6b11a92e5f}");

enum AcquireT { acquire };
class MutexReleaser {
  public:
    MutexReleaser(HANDLE mutex) : mutex_(mutex) {}
    MutexReleaser(HANDLE mutex, AcquireT) : mutex_(mutex) {
      for (;;) {
        DWORD ret_val = WaitForSingleObject(mutex_, INFINITE);
        // with an infinite wait time should never get WAIT_TIMEOUT
        ASSERT(ret_val != WAIT_TIMEOUT);
        if (ret_val == WAIT_FAILED) {
          WIN_EXCEPT("Failed call to WaitForSingleObject(). ");
        } else if (ret_val == WAIT_ABANDONED) {
          // if the mutex was abandoned it should be unsignaled, try to acquire again
          continue;
        } else {
          ASSERT(ret_val == WAIT_OBJECT_0);
          return;
        }
      }
    }
    ~MutexReleaser() { 
      ReleaseMutex(mutex_); 
    }
  private:
    HANDLE mutex_;
    MutexReleaser(const MutexReleaser &);
    MutexReleaser & operator=(const MutexReleaser &);
};

// Wraps a file mapping handle to be a HWND reference
class MMapHWND {
  public:
    MMapHWND(void) : master_(false) {
      HANDLE file_mapping_handle = CreateFileMapping(INVALID_HANDLE_VALUE, // no backing file
                                                     0,
                                                     PAGE_READWRITE,
                                                     0,
                                                     sizeof(HWND),
                                                     MMAP_NAME);
      if (file_mapping_handle == NULL) WIN_EXCEPT("Failure in CreateFileMapping() call.");
      file_mapping_.Attach(file_mapping_handle);

      HANDLE mutex = CreateMutex(NULL, FALSE, MMAP_MUTEX_NAME);
      if (mutex == NULL) WIN_EXCEPT("Failed call to CreateMutex(). ");
      mutex_.Attach(mutex);

      void * ptr = MapViewOfFile(file_mapping_handle, FILE_MAP_WRITE, 0, 0, sizeof(HWND));
      if (!ptr) WIN_EXCEPT("Failure in MapViewOfFile() call.");
      ptr_ = reinterpret_cast<HWND *>(ptr);
    }
    ~MMapHWND() {
      // Null out the hwnd so that a process starting as this one closes doesn't
      //   try to connect to a dead window.
      if (master_) {
        MutexReleaser(mutex_, acquire);
        *ptr_ = 0;
      }
      BOOL r = UnmapViewOfFile(ptr_);
      ASSERT(r);
    }
    HWND get(void) const {
      HWND to_return;
      { MutexReleaser releaser(mutex_, acquire);
        to_return = *ptr_;
      }
      return to_return; 
    }
    void set(HWND hWnd) {
      ASSERT(!master_);
      MutexReleaser releaser(mutex_, acquire);
      ASSERT(*ptr_ == 0);
      *ptr_ = hWnd;
      master_ = true;
    }
  private:
    CHandle file_mapping_;
    CHandle mutex_;
    HWND * ptr_;
    bool master_;

    MMapHWND(const MMapHWND &);
    MMapHWND & operator=(const MMapHWND &);
};

MsgDataPtr get_message_data(bool adjust, const TCHAR * cmd_line, const TCHAR * working_directory) {
  size_t cmd_line_length = _tcslen(cmd_line) + 1;
  size_t working_directory_length = _tcslen(working_directory) + 1;
  size_t size = sizeof(MessageData) + sizeof(TCHAR) * (cmd_line_length + working_directory_length);
  MessageData * msg = (MessageData *)malloc(size);
  if (!msg) MISC_EXCEPT("Error allocating memory for message data. ");
  msg->size = size;
  msg->console_window = NULL;
  msg->adjust = adjust;
  msg->cmd_line_length = cmd_line_length;
  msg->working_directory_length = working_directory_length;
  _tcscpy(msg->char_data, cmd_line);
  _tcscpy(&(msg->char_data[cmd_line_length]), working_directory);
  return MsgDataPtr(msg, &free);
}

int try_print_help(void) {
  BOOL r = AttachConsole(ATTACH_PARENT_PROCESS);
  if (r) {
    if (!freopen("CONOUT$", "w", stdout)) MISC_EXCEPT("Error opening console output. ");
    print_help();
    if (!FreeConsole()) WIN_EXCEPT("Failed call to FreeConsole(). ");
  }
  return 0;
}

int send_data(bool adjust, LPCTSTR lpCmdLine, HWND hWnd) {
  ASSERT(hWnd != 0);
  std::unique_ptr<TCHAR [], void (*)(void *)> working_directory(_tgetcwd(nullptr, 0), free);
  MsgDataPtr msg_data = get_message_data(adjust, lpCmdLine, working_directory.get());
  COPYDATASTRUCT cds = {
    0,
    static_cast<DWORD>(msg_data->size),
    &*msg_data
  };
  BOOL r = AttachConsole(ATTACH_PARENT_PROCESS);
  if (r) {
    msg_data->console_window = GetConsoleWindow();
    SendMessage(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
    FreeConsole();
  } else {
    if (adjust) {
      MessageBox(NULL,
                 _T("There doesn't seem to be an associated conrep window to adjust"), 
                 _T("--adjust error"), 
                 MB_OK);
    } else {
      SendMessage(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
    }
  }
  return 0;
}

// actual logic for winmain; separates out C++ exception handling from SEH exception
//   handling 
int do_winmain1(HINSTANCE hInstance, 
                LPTSTR lpCmdLine, 
                const tstring & exe_dir,
                tstring & message,
                bool & execute_filter) {
  CommandLineOptions opt(lpCmdLine);
  if (opt.help) {
    return try_print_help();
  }

  // Use a mutex object to ensure only one instance of the application is active
  //   at a time; this allows different console windows to share resources like
  //   background textures
  HANDLE mutex_handle = CreateMutex(0, TRUE, APP_MUTEX_NAME);
  if (!mutex_handle) WIN_EXCEPT("Failed call to CreateMutex(). ");
  CHandle mutex(mutex_handle);
  DWORD err = GetLastError();

  MMapHWND mmap;
  if (err == ERROR_ALREADY_EXISTS) {
    for (;;) {
      // If two instances of the program are started close to each other then one could
      //   try reading the HWND value before it is set. Fortunately the shared memory
      //   area is guaranteed to be zeroed, and zero isn't a valid HWND value.
      HWND hWnd = mmap.get();
      if (hWnd != 0) {
        return send_data(opt.adjust, lpCmdLine, hWnd);
      }
      DWORD ret_val = WaitForSingleObject(mutex, 100);
      if (ret_val == WAIT_ABANDONED) {
        // Original program closed before assigning an hwnd value. Mutex is now
        //   unsignaled. Do another iteration through the loop and attempt to
        //   acquire mutex ownership.
        continue;
      } else if (ret_val == WAIT_OBJECT_0) {
        // Got the mutex, now the master process. Break out of the loop and go
        //   through window creation.
        break;
      } else if (ret_val == WAIT_TIMEOUT) {
        // Call timed out before either the existing process closing or
        //   obtaining the mutex. Do another iteration through the loop.
        continue;
      } else {
        ASSERT(ret_val == WAIT_FAILED);
        WIN_EXCEPT("Failed call to WaitForSingleObject().");
      }
    }
  }
  MutexReleaser releaser(mutex);

  if (opt.adjust) {
    MessageBox(NULL, _T("No existing conrep window to adjust."), _T("--adjust error"), MB_OK);
    return 0;
  }

  GDIPlusInit gdi_initializer;
  COMInit com_initializer;
  
  RootWindowPtr root_window(get_root_window(hInstance, exe_dir, message));
  std::unique_ptr<TCHAR [], void (*)(void *)> working_directory(_tgetcwd(nullptr, 0), free);
  Settings settings(lpCmdLine, exe_dir.c_str(), working_directory.get());
  execute_filter = settings.execute_filter;

  if (!root_window->spawn_window(settings)) return 0;
  mmap.set(root_window->hwnd()); // place window handle in shared memory

  for (;;) {
    MSG msg;
    // rumors of the return value being actually boolean are
    //   greatly exaggerated. GetMessage() actually returns
    //   a tri-state value
    BOOL bRet = GetMessage(&msg, 0, 0, 0);
    if (bRet == 0) {
      return static_cast<int>(msg.wParam);
    } else if (bRet == -1) {
      // some sort of error condition
      WIN_EXCEPT("Error in message pump.");
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}


// SEH exception handler function. Since C++ objects that require unwind semantics
//   aren't allowable in function with SEH __try blocks, message is created by the
//   caller and assigned in the filter by reference.
int WINAPI do_winmain2(HINSTANCE hInstance,
                       LPTSTR lpCmdLine,
                       const tstring & exe_dir,
                       tstring & message) {
  bool execute_filter = true;
  __try {
    int ret_val = do_winmain1(hInstance, lpCmdLine, exe_dir, message, execute_filter);
    if (ret_val == EXIT_ABNORMAL_TERMINATION) {
      if (!message.empty()) {
        MessageBox(NULL,  message.c_str(), _T("Abnormal termination"), MB_OK); 
      }
    }
    return ret_val;
  // exception_flter() doesn't handle stack overflow properly, so just skip the
  //   the function and dump to the OS exception handler in that case.
  } __except( (execute_filter && (GetExceptionCode() != EXCEPTION_STACK_OVERFLOW))
                ? exception_filter(exe_dir, *GetExceptionInformation(), message)
                : EXCEPTION_CONTINUE_SEARCH
             ) {
    if (!message.empty()) {
      MessageBox(NULL, message.c_str(), _T("Abnormal termination"), MB_OK); 
    }
    return EXIT_ABNORMAL_TERMINATION;
  }
}

// _tWinMain sets up objects necessary for exception handling and then calls
//   do_winmain2(); do_winmain2() then reports any unhandled exceptions and calls
//   do_winmain1(), which performs actual program logic. Bizarre structure due to
//   the limitation of being unable to use C++ exceptions or objects with
//   destructors in a function with SEH exception handling
#pragma warning(suppress: 28251)
int WINAPI _tWinMain(HINSTANCE hInstance,
                     HINSTANCE,
                     LPTSTR lpCmdLine,
                     int) {
  _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  disable_process_callback_filter();
  tstring exe_dir = get_branch_from_path(get_module_path());
  SymInit sym;
  tstring message;
  return do_winmain2(hInstance, lpCmdLine, exe_dir, message);
}