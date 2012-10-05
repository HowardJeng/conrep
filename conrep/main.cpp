// main.cpp
// contains the entry point and initialization for application

#include "windows.h"
#include <dbghelp.h>

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

// actual logic for winmain; separates out C++ exception handling from SEH exception
//   handling 
int do_winmain1(HINSTANCE hInstance, 
                HINSTANCE,
                LPTSTR lpCmdLine, 
                int,
                const tstring & exe_dir,
                bool & execute_filter) {
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
        COPYDATASTRUCT cds = {
          0,
          static_cast<DWORD>((_tcslen(lpCmdLine) + 1) * sizeof(TCHAR)),
          lpCmdLine
        };
        SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
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

  GDIPlusInit gdi_initializer;
  COMInit com_initializer;
  
  std::auto_ptr<IRootWindow> root_window(get_root_window(hInstance, exe_dir));
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

DWORD do_exception_filter(const tstring & exe_dir,
                          EXCEPTION_POINTERS * eps,
                          tstring & message) {
  int skip = 0;
  std::stringstream narrow_stream;
  EXCEPTION_RECORD * er = eps->ExceptionRecord;
  switch (er->ExceptionCode) {
    case MSC_EXCEPTION_CODE: { // C++ exception
      UntypedException ue(*eps);
      if (std::exception * e = exception_cast<std::exception>(ue)) {
        narrow_stream << typeid(*e).name() << "\n" << e->what();
      } else {
        narrow_stream << "Unknown C++ exception thrown.\n";
        get_exception_types(narrow_stream, ue);
      }
      skip = 2; // skips RaiseException() and _CxxThrowException()
    } break;
    case ASSERT_EXCEPTION_CODE: {
      char * assert_message = reinterpret_cast<char *>(er->ExceptionInformation[0]);
      narrow_stream << assert_message;
      free(assert_message);
      skip = 1; // skips RaiseException()
    } break;
    case EXCEPTION_ACCESS_VIOLATION: {
      narrow_stream << "Access violation. Illegal "
                    << (er->ExceptionInformation[0] ? "write" : "read")
                    << " by "
                    << er->ExceptionAddress
                    << " at "
                    << reinterpret_cast<void *>(er->ExceptionInformation[1]);
    } break;
    default: {
      narrow_stream << "SEH exception thrown. Exception code: "
                    << std::hex << std::uppercase << er->ExceptionCode
                    << " at "
                    << er->ExceptionAddress;
    }
  }
  narrow_stream << "\n\nStack Trace:\n";
  generate_stack_walk(narrow_stream, *eps->ContextRecord, skip);

  tstringstream sstr;
  sstr << TBuffer(narrow_stream.str().c_str());

  std::ofstream error_log(NarrowBuffer((exe_dir + _T("\\err_log.txt")).c_str()));
  if (error_log) {
    error_log << narrow_stream.str();
    sstr << _T("\nThis message has been written to err_log.txt");
  }
  
  message = sstr.str();
  return EXCEPTION_EXECUTE_HANDLER;
}

DWORD exception_filter(const tstring & exe_dir,
                       EXCEPTION_POINTERS * eps,
                       tstring & message) {
  // in case of errors like a corrupted heap or stack or other error condition that
  //   prevents the stack trace from being built, just dump to the operating system
  //   exception handler and hope that the filter didn't mess up the program state
  //   sufficiently to make a crash dump useless
  __try {
    return do_exception_filter(exe_dir, eps, message);
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    return EXCEPTION_CONTINUE_SEARCH;
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
    return do_winmain1(hInstance, hPrevInstance, lpCmdLine, nCmdShow, exe_dir, execute_filter);
  // exception_flter() doesn't handle stack overflow properly, so just skip the
  //   the function and dump to the OS exception handler in that case.
  } __except( (execute_filter && (GetExceptionCode() != EXCEPTION_STACK_OVERFLOW))
                ? exception_filter(exe_dir, GetExceptionInformation(), message)
                : EXCEPTION_CONTINUE_SEARCH
             ) {
    if (!message.empty()) {
      MessageBox(NULL, 
                 message.c_str(),
                 _T("Abnormal termination"),
                 MB_OK); 
    }
    return -1;
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
  tstring exe_dir = get_branch_from_path(get_module_path());
  SymInit sym;
  tstring message;
  return do_winmain2(hInstance, hPrevInstance, lpCmdLine, nCmdShow, exe_dir, message);
}