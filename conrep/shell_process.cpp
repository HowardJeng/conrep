// shell_process.cpp
// implementation of ShellProcess class for communication with spawned console windows

#include "shell_process.h"

#include <functional>

#include "assert.h"
#include "atl.h"
#include "char_info_buffer.h"
#include "dimension.h"
#include "dimension_ops.h"
#include "exception.h"
#include "settings.h"

using namespace ATL;

namespace console {
  int ShellProcess::attach_count_ = 0;

  class Cleaner {
    public:
      typedef void (ShellProcess::*MemFn)(void);

      Cleaner(MemFn fn, ShellProcess * obj) : fn_(fn), obj_(obj) {}
      ~Cleaner() { (obj_->*fn_)(); }
    private:
      MemFn fn_;
      ShellProcess * obj_;

      Cleaner(const Cleaner &);
      Cleaner & operator=(const Cleaner &);
  };

  ShellProcess::ShellProcess(Settings & settings)
    : process_id_(0),
      window_handle_(0),
      #ifdef DEBUG
        console_visible_(true)
      #else
        console_visible_(false)
      #endif
  {
    ASSERT(!settings.shell.empty());
    ASSERT(!attach_count_);
      
    // Allocate a new console window. This window will be eventually owned by the new
    //   shell process.
    if (!AllocConsole()) WIN_EXCEPT("Failed call to AllocConsole(). ");
    ++attach_count_;
    {
      Cleaner cl(&ShellProcess::detach, this);
      create_shell_process(settings);
    }
    ASSERT(!attach_count_);
  }
    
  void ShellProcess::create_shell_process(Settings & settings) {
    tstring command_line(settings.shell);
    if (!settings.shell_arguments.empty()) {
      command_line += _T(" ");
      command_line += settings.shell_arguments;
    }
    // CreateProcess() requires lpCommandLine to be a non-const TCHAR buffer. It
    //   actually does modify the buffer, so a const_cast is not a viable option.
    std::vector<TCHAR> buffer(command_line.begin(), command_line.end());
    buffer.push_back(0);

    PROCESS_INFORMATION pi = {};
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    // The new process will be created attached to the just allocated console window.
    //   Do not create with CREATE_NEW_CONSOLE and call AttachConsole() as then the
    //   created process is not guranteed to have created the new console before
    //   AttachConsole() call is executed.
    if (!CreateProcess(0, &buffer[0], 0, 0, TRUE, 0, 0, 0, &si, &pi))
      WIN_EXCEPT("Unable to spawn child process in ShellProcess constructor. ");

    ASSERT(pi.hProcess != NULL);
    ASSERT(pi.hThread != NULL);
    process_handle_.Attach(pi.hProcess);
    process_id_ = pi.dwProcessId;
    if (!CloseHandle(pi.hThread)) WIN_EXCEPT("CloseHandle() failed on thread handle. ");

    window_handle_ = GetConsoleWindow();
    #ifndef DEBUG
      ShowWindow(window_handle_, SW_HIDE);
    #endif
      
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout_handle == INVALID_HANDLE_VALUE) WIN_EXCEPT("Unable to get stdout handle in ShellProcess constructor. ");
    stdout_handle_.Attach(stdout_handle);
      
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(stdout_handle, &csbi)) WIN_EXCEPT("Failed call to GetConsoleScreenBufferInfo() ");
      
    COORD size = GetLargestConsoleWindowSize(stdout_handle);
    if (!size.X && !size.Y) WIN_EXCEPT("Failed call to GetLargestConsoleWindowSize() ");
    if (settings.rows < 0) settings.rows = size.Y;
    else settings.rows = std::min<int>(size.Y, settings.rows);
    if (settings.columns < 0) settings.columns = size.X;
    else settings.columns = std::min<int>(size.X, settings.columns);
      
    COORD buffer_size = { static_cast<short>(settings.columns), 
                          static_cast<short>(std::max<int>(csbi.dwSize.Y, settings.rows)) };
    if (!SetConsoleScreenBufferSize(stdout_handle, buffer_size)) WIN_EXCEPT("Failed call to SetConsoleScreenBufferSize() ");
    SMALL_RECT viewport = { 0, 
                            0, 
                            static_cast<short>(settings.columns - 1), 
                            static_cast<short>(settings.rows - 1) };
    if (!SetConsoleWindowInfo(stdout_handle, TRUE, &viewport)) WIN_EXCEPT("Failed call to SetConsoleWindowInfo() ");
  }

  ShellProcess::~ShellProcess() {
    PostMessage(window_handle_, WM_CLOSE, 0, 0);
  }

  bool ShellProcess::attached(void) {
    return attach_count_ != 0;
  }
        
  HWND ShellProcess::window_handle(void) const {
    ASSERT(window_handle_);
    return window_handle_;
  }

  HANDLE ShellProcess::process_handle(void) const {
    ASSERT(process_handle_ != NULL);
    return process_handle_;
  }
      
  bool ShellProcess::is_console_visible(void) const {
    return console_visible_;
  }

  void ShellProcess::toggle_console_visible(void) {
    console_visible_ = !console_visible_;
    // ShowWindow()'s return value doesn't contain error information so can be ignored
    if (console_visible_) {
      ShowWindow(window_handle_, SW_SHOW);
    } else {
      ShowWindow(window_handle_, SW_HIDE);
    }
  }

  bool ShellProcess::attach(void) {
    ++attach_count_;
    if (attach_count_ == 1) {
      while (!AttachConsole(process_id_)) {
        // I don't know why, but sometimes AttachConsole() can say the process has closed when it hasn't. So
        //   ignore errors that say it has and check if the process has closed another way.
        DWORD err = GetLastError();
		if (err == 5) return false;
        if (err != 31) {
          WIN_EXCEPT2("Failed call to AttachConsole(). ", err);
        }
        DWORD ret = WaitForSingleObject(process_handle_, 0);
        if (ret == WAIT_OBJECT_0) {
          // a signalled process handle indicates process has closed
          return false;
        } else if (ret == WAIT_FAILED) {
          WIN_EXCEPT("Failed call to WaitForSingleObject(). ");
        }
      }
    }
    HWND console_window = GetConsoleWindow();
    if (!console_window) WIN_EXCEPT("Failed call to GetConsoleWindow(). ");
    ASSERT(console_window == window_handle_);
    if (console_window != window_handle_) MISC_EXCEPT("Fatal Error: double console attachment.");
    return true;
  }

  void ShellProcess::detach(void) {
    ASSERT(attach_count_);
    --attach_count_;
    if (!attach_count_) FreeConsole();
  }
        
    
  Dimension ShellProcess::resize(Dimension console_dim) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(stdout_handle_, &csbi)) WIN_EXCEPT("Failed call to GetConsoleScreenBufferInfo(). ");

    int current_width  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
      
    COORD max_size = GetLargestConsoleWindowSize(stdout_handle_);
    if (!max_size.X && !max_size.Y) WIN_EXCEPT("Failed call to GetLargestConsoleWindowSize(). ");
    if ((console_dim.width  > max_size.X) || (console_dim.width < 0)) console_dim.width  = max_size.X;
    if ((console_dim.height > max_size.Y) || (console_dim.height < 0)) console_dim.height = max_size.Y;
    int new_height = std::max<int>(console_dim.height, csbi.dwSize.Y);

    COORD buffer_size = { static_cast<short>(console_dim.width), 
                          static_cast<short>(new_height) };
    SMALL_RECT viewport = { 0, 
                            0, 
                            static_cast<short>(console_dim.width - 1), 
                            static_cast<short>(console_dim.height - 1) };
    if (current_width < console_dim.width) {
      if (!SetConsoleScreenBufferSize(stdout_handle_, buffer_size)) WIN_EXCEPT("Failed call to SetConsoleScreenBufferSize(). ");
      if (!SetConsoleWindowInfo(stdout_handle_, TRUE, &viewport)) WIN_EXCEPT("Failed call to SetConsoleWindowInfo(). ");
    } else {
      if (!SetConsoleWindowInfo(stdout_handle_, TRUE, &viewport)) WIN_EXCEPT("Failed call to SetConsoleWindowInfo(). ");
      if (!SetConsoleScreenBufferSize(stdout_handle_, buffer_size)) WIN_EXCEPT("Failed call to SetConsoleScreenBufferSize(). ");
    }

    if (!GetConsoleScreenBufferInfo(stdout_handle_, &csbi)) WIN_EXCEPT("Failed call to GetConsoleScreenBufferInfo(). ");
    console_dim.width  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    console_dim.height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    return console_dim;
  }
    
  void ShellProcess::get_console_info(const Dimension & console_dim, CharInfoBuffer & buffer, COORD & cursor_pos) {
    ASSERT(attach_count_);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(stdout_handle_, &csbi)) WIN_EXCEPT("Failed call to GetConsoleScreenBufferInfo(). ");

    COORD size = { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
    COORD origin = {};

    size_t required_size = size.X * size.Y;

    Dimension size_dim = Dimension(size.X, size.Y);
    if (size_dim != console_dim) {
      buffer.resize(size_dim);
    }
      
    // ReadConsoleOutput() can only read 64K at a time
    if (required_size * sizeof(CHAR_INFO) < 64 * 1024) {
      if (!ReadConsoleOutput(stdout_handle_, &buffer[0], size, origin, &csbi.srWindow))
        WIN_EXCEPT("Failed call to ReadConsoleOutput(). ");
    } else {
      // read in one line at a time
      COORD line_size = { size.X, 1 };
      for (short i = 0; i < size.Y; i++) {
        SMALL_RECT sr = { csbi.srWindow.Left,
                          csbi.srWindow.Top + i,
                          csbi.srWindow.Right,
                          csbi.srWindow.Top + i };
        if (!ReadConsoleOutput(stdout_handle_, &buffer[i * size.X], line_size, origin, &sr))
          WIN_EXCEPT("Failed call to ReadConsoleOutput(). ");
      }
    }
      
    cursor_pos.X = csbi.dwCursorPosition.X;
    cursor_pos.Y = csbi.dwCursorPosition.Y - csbi.srWindow.Top;
  }

  Dimension ShellProcess::get_console_size(void) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(stdout_handle_, &csbi)) {
      DWORD err = GetLastError();
      if (err == 6) {
        Sleep(0);
        reset_handle();
        if (!GetConsoleScreenBufferInfo(stdout_handle_, &csbi)) WIN_EXCEPT("Failed call to GetConsoleScreenBufferInfo(). ");
      } else {
        WIN_EXCEPT2("Failed call to GetConsoleScreenBufferInfo(). ", err);
      }
    }

    return Dimension(csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
  }
    
  void ShellProcess::reset_handle(void) {
    stdout_handle_.Detach();
    HANDLE stdout_handle = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (stdout_handle == INVALID_HANDLE_VALUE) WIN_EXCEPT("Failed call to GetStdHandle(). ");
    stdout_handle_.Attach(stdout_handle);
  }
  
  ProcessLock::ProcessLock(ShellProcess & shell_process)
    : shell_process_(shell_process),
      attached_(shell_process.attach())
  {}
  ProcessLock::~ProcessLock() {
    shell_process_.detach();
  }

  Dimension ProcessLock::resize(Dimension console_dim) {
    ASSERT(attached_);
    return shell_process_.resize(console_dim);
  }
  void ProcessLock::get_console_info(const Dimension & console_dim, CharInfoBuffer & buffer, COORD & cursor_pos) {
    ASSERT(attached_);
    shell_process_.get_console_info(console_dim, buffer, cursor_pos);
  }
      
  Dimension ProcessLock::get_console_size(void) {
    ASSERT(attached_);
    return shell_process_.get_console_size();
  }

  ProcessLock::operator bool(void) const {
    return attached_;
  }
}