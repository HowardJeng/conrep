// Interface for interacting with the shell process and its window
#ifndef CONREP_SHELL_PROCESS_H
#define CONREP_SHELL_PROCESS_H

#include "windows.h"

#include <memory>
#include "atl.h"
#include "tchar.h"

namespace console {
  struct Dimension;
  struct Settings;
  class CharInfoBuffer;
  class ProcessLock;
  
  // ProcessLock should be considered to be part of the public interface of 
  //   IShellProcess. Since there are function calls in the IShellProcess
  //   that should only be called between attach() and detach() calls, we
  //   enforce that restriction with the type system by making those calls
  //   only possible through the ProcessLock class. It also guarantees the
  //   call of IShellProcess::detach() in the event of an exception.
  class ShellProcess {
    public:
      ShellProcess(Settings & settings);
      ~ShellProcess();

      bool attached(void);
        
      HWND window_handle(void) const;
      HANDLE process_handle(void) const;
    private:
      ShellProcess(const ShellProcess &);
      ShellProcess & operator=(const ShellProcess &);
        
      CHandle process_handle_;
      CHandle stdout_handle_;
      DWORD   process_id_;
      HWND    window_handle_;
        
      static int attach_count_;

      bool attach(void);
      void detach(void);
        
      void do_construct1(Settings & settings);
      void do_construct2(Settings & settings);
        
      void get_console_info(const Dimension & console_dim, CharInfoBuffer & buffer, COORD & cursor_pos);
      Dimension resize(Dimension console_dim);

      Dimension get_console_size(void);
        
      void reset_handle(void);

      friend ProcessLock;
  };
  
  class ProcessLock {
    public:
      ProcessLock(ShellProcess & shell_process);
      ~ProcessLock();

      Dimension resize(Dimension console_dim);
      void get_console_info(const Dimension & console_dim, CharInfoBuffer & buffer, COORD & cursor_pos);
      Dimension get_console_size(void);

      operator bool(void) const;
    private:
      ShellProcess & shell_process_;
      bool attached_;
      
      ProcessLock(const ProcessLock &);
      ProcessLock & operator=(const ProcessLock &);
  };
}

#endif