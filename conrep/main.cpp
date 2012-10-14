// main.cpp
// contains the entry point and initialization for application

#include "windows.h"
#include <dbghelp.h>

#include <iostream>
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

  HMODULE kernel32 = LoadLibraryA("kernel32.dll");
  if (!kernel32) return;
  GetPolicyPtr get_policy = (GetPolicyPtr)GetProcAddress(kernel32, "GetProcessUserModeExceptionPolicy"); 
  if (!get_policy) return;
  SetPolicyPtr set_policy = (SetPolicyPtr)GetProcAddress(kernel32, "SetProcessUserModeExceptionPolicy"); 
  if (!set_policy) return;

  DWORD flags; 
  if (get_policy(&flags)) { 
    set_policy(flags & ~PROCESS_CALLBACK_FILTER_ENABLED); 
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

// Wraps a file mapping handle to be a HWND reference
class MMapHWND {
  public:
    MMapHWND(HANDLE mapping) : master_(false) {
      void * ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, sizeof(HWND));
      if (!ptr) WIN_EXCEPT("Failure in MapViewOfFile() call.");
      ptr_ = reinterpret_cast<HWND *>(ptr);
    }
    ~MMapHWND() {
      // Null out the hwnd so that a process starting as this one closes doesn't
      //   try to connect to a dead window.
      if (master_) *ptr_ = 0; 
      BOOL r = UnmapViewOfFile(const_cast<HWND *>(ptr_));
      ASSERT(r);
    }
    operator HWND() const { return *ptr_; }
    MMapHWND & operator=(HWND rhs) { 
      *ptr_ = rhs; 
      master_ = true; 
      return *this; 
    }
  private:
    volatile HWND * ptr_;
    bool master_;
};

// Using session namespace kernal objects will allow different users to run
//   separate instances under XP fast user switching. This may not be a good idea
//   as this application is a huge resource hog.
const TCHAR MUTEX_NAME[] = _T("Local\\conrep{7c1123af-2ffe-41e7-aebd-da66f803aca7}");
const TCHAR MMAP_NAME[]  = _T("Local\\conrep{4b581c5e-5f5a-4fb7-b185-1252cea83d92}");


MsgDataPtr get_message_data(bool adjust, const TCHAR * cmd_line) {
  size_t length = _tcslen(cmd_line);
  size_t size = sizeof(MessageData) + sizeof(TCHAR) * length;
  MessageData * msg = (MessageData *)malloc(size);
  if (!msg) MISC_EXCEPT("Error allocating memory for message data. ");
  msg->size = size;
  msg->console_window = NULL;
  msg->adjust = adjust;
  _tcscpy(msg->cmd_line, cmd_line);
  return MsgDataPtr(msg, &free);
}

// actual logic for winmain; separates out C++ exception handling from SEH exception
//   handling 
int do_winmain1(HINSTANCE hInstance, 
                HINSTANCE,
                LPTSTR lpCmdLine, 
                int,
                const tstring & exe_dir,
                tstring & message,
                bool & execute_filter) {
  CommandLineOptions opt(lpCmdLine);
  if (opt.help) {
    BOOL r = AttachConsole(ATTACH_PARENT_PROCESS);
    if (r) {
      if (!freopen("CONOUT$", "w", stdout)) MISC_EXCEPT("Error opening console output. ");
      print_help();
      if (!FreeConsole()) WIN_EXCEPT("Failed call to FreeConsole(). ");
    }
    return 0;
  }

  // Use a mutex object to ensure only one instance of the application is active
  //   at a time; this allows different console windows to share resources like
  //   background textures
  HANDLE mutex_handle = CreateMutex(0, TRUE, MUTEX_NAME);
  if (!mutex_handle) WIN_EXCEPT("Failed call to CreateMutex(). ");
  CHandle mutex(mutex_handle);
  DWORD err = GetLastError();

  HANDLE file_mapping_handle = CreateFileMapping(INVALID_HANDLE_VALUE, // no backing file
                                                 0,
                                                 PAGE_READWRITE,
                                                 0,
                                                 sizeof(HWND),
                                                 MMAP_NAME);
  if (file_mapping_handle == NULL) WIN_EXCEPT("Failure in CreateFileMapping() call.");
  CHandle file_mapping(file_mapping_handle);

  MMapHWND hwnd(file_mapping);
  if (err == ERROR_ALREADY_EXISTS) {
    for (;;) {
      // If two instances of the program are started close to each other then one could
      //   try reading the HWND value before it is set. Fortunately the shared memory
      //   area is guaranteed to be zeroed, and zero isn't a valid HWND value.
      if (hwnd != 0) {
        MsgDataPtr msg_data = get_message_data(opt.adjust, lpCmdLine);
        COPYDATASTRUCT cds = {
          0,
          static_cast<DWORD>(msg_data->size),
          &*msg_data
        };
        BOOL r = AttachConsole(ATTACH_PARENT_PROCESS);
        if (r) {
          msg_data->console_window = GetConsoleWindow();
          SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
          FreeConsole();
        } else {
          if (opt.adjust) {
            MessageBox(NULL, _T("There doesn't seem to be an associated conrep window to adjust"), _T("--adjust error"), MB_OK);
          } else {
            SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
          }
        }
        return 0;
      } else {
        DWORD ret_val = WaitForSingleObject(mutex, 100);
        if (ret_val == WAIT_ABANDONED) {
          // Original program closed before assigning an hwnd value. Mutex is now
          //   unsignaled. Do another iteration through the loop and attempt to
          //   acquire mutex ownership.
        } else if (ret_val == WAIT_OBJECT_0) {
          // Got the mutex, now the master process. Break out of the loop and go
          //   through window creation.
          break;
        } else if (ret_val == WAIT_TIMEOUT) {
          // Call timed out before either the existing process closing or
          //   obtaining the mutex. Do another iteration through the loop.
        } else {
          ASSERT(ret_val == WAIT_FAILED);
          WIN_EXCEPT("Failed call to WaitForSingleObject().");
        }
      }
    }
  }

  if (opt.adjust) {
    MessageBox(NULL, _T("No existing conrep window to adjust."), _T("--adjust error"), MB_OK);
    return 0;
  }

  GDIPlusInit gdi_initializer;
  COMInit com_initializer;
  
  std::auto_ptr<IRootWindow> root_window(get_root_window(hInstance, exe_dir, message));
  Settings settings(lpCmdLine, exe_dir.c_str());
  execute_filter = settings.execute_filter;

  if (!root_window->spawn_window(settings)) return 0;
  hwnd = root_window->hwnd(); // place window handle in shared memory

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
                       HINSTANCE hPrevInstance,
                       LPTSTR lpCmdLine,
                       int nCmdShow,
                       const tstring & exe_dir,
                       tstring & message) {
  bool execute_filter = true;
  __try {
    int ret_val = do_winmain1(hInstance, hPrevInstance, lpCmdLine, nCmdShow, exe_dir, message, execute_filter);
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
                     HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine,
                     int nCmdShow) {
  disable_process_callback_filter();
  tstring exe_dir = get_branch_from_path(get_module_path());
  SymInit sym;
  tstring message;
  return do_winmain2(hInstance, hPrevInstance, lpCmdLine, nCmdShow, exe_dir, message);
}