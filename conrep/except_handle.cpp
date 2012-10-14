#include "except_handle.h"

#include <dbghelp.h>
#include "file_util.h"

#include <fstream>
#include <sstream>

namespace console {
  #ifdef _M_IX86
    void * exception_cast_worker(const UntypedException & e, const type_info & ti) {
      for (int i = 0; i < e.type_array->nCatchableTypes; ++i) {
        _CatchableType & type_i = *e.type_array->arrayOfCatchableTypes[i];
        const std::type_info & ti_i = *reinterpret_cast<std::type_info *>(type_i.pType);
        if (ti_i == ti) {
          char * base_address = reinterpret_cast<char *>(e.exception_object);
          base_address += type_i.thisDisplacement.mdisp;
          return base_address;
        }
      }
      return 0;
    }
   
    void get_exception_types(std::ostream & os, const UntypedException & e) {
      for (int i = 0; i < e.type_array->nCatchableTypes; ++i) {
        _CatchableType & type_i = *e.type_array->arrayOfCatchableTypes[i];
        const std::type_info & ti_i = *reinterpret_cast<std::type_info *>(type_i.pType);
        os << ti_i.name() << "\n";
      }
    }
  #elif defined(_M_X64) && (_MSC_VER >= 1400)
    void * exception_cast_worker(const UntypedException & e, const type_info & ti) {
      if (_is_exception_typeof(ti, e.exception_pointers)) {
        return reinterpret_cast<void *>((e.exception_pointers->ExceptionRecord->ExceptionInformation)[1]);
      }
      return 0;
    }
   
    void get_exception_types(std::ostream &, const UntypedException &) {
    }
  #else
    #error Unsupported platform
  #endif 

  
  void write_module_name(std::ostream & os, HANDLE process, DWORD64 program_counter) {
    DWORD64 module_base = SymGetModuleBase64(process, program_counter);
    if (module_base) {
      std::string module_name = get_module_path_a(reinterpret_cast<HMODULE>(module_base));
      if (!module_name.empty())
        os << get_leaf_from_path(module_name) << "|";
      else 
        os << "Unknown module(" << GetLastError() << ")|";
    } else {
      os << "Unknown module(" << GetLastError() << ")|";
    }
  }
  
  void write_function_name(std::ostream & os, HANDLE process, DWORD64 program_counter) {
    SYMBOL_INFO_PACKAGE sym = { sizeof(SYMBOL_INFO) };
    sym.si.MaxNameLen = MAX_SYM_NAME;
    if (SymFromAddr(process, program_counter, 0, &sym.si)) {
      os << sym.si.Name << "()";
    } else {
      os << "Unknown function(" << GetLastError() << ")";
    } 
  }
  
  void write_file_and_line(std::ostream & os, HANDLE process, DWORD64 program_counter) {
    IMAGEHLP_LINE64 ih_line = { sizeof(IMAGEHLP_LINE64) };
    DWORD dummy = 0;
    if (SymGetLineFromAddr64(process, program_counter, &dummy, &ih_line)) {
      os << "|" << get_leaf_from_path(ih_line.FileName)
         << ":" << ih_line.LineNumber;
    }
  }
  
  void generate_stack_walk(std::ostream & os, CONTEXT ctx, int skip) {
    STACKFRAME64 sf = {};
    #ifdef _M_IX86
      DWORD machine_type  = IMAGE_FILE_MACHINE_I386;
      sf.AddrPC.Offset    = ctx.Eip;
      sf.AddrPC.Mode      = AddrModeFlat;
      sf.AddrStack.Offset = ctx.Esp;
      sf.AddrStack.Mode   = AddrModeFlat;
      sf.AddrFrame.Offset = ctx.Ebp;
      sf.AddrFrame.Mode   = AddrModeFlat;
    #elif defined(_M_X64)
	    DWORD machine_type  = IMAGE_FILE_MACHINE_AMD64;
	    sf.AddrPC.Offset    = ctx.Rip;
	    sf.AddrPC.Mode      = AddrModeFlat;
	    sf.AddrFrame.Offset = ctx.Rsp;
	    sf.AddrFrame.Mode   = AddrModeFlat;
	    sf.AddrStack.Offset = ctx.Rsp;
	    sf.AddrStack.Mode   = AddrModeFlat;
	  #else
	    #error Unsupported platform
	  #endif
     
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    os << std::uppercase;
    for (;;) {
      SetLastError(0);
      BOOL stack_walk_ok = StackWalk64(machine_type, process, thread, &sf,
                                      &ctx, 0, &SymFunctionTableAccess64, 
                                      &SymGetModuleBase64, 0);
      if (!stack_walk_ok || !sf.AddrFrame.Offset) return;
      
      if (skip) {
        --skip;
      } else {
        // write the address
        os << std::hex << reinterpret_cast<void *>(sf.AddrPC.Offset) << "|" << std::dec;

        write_module_name(os, process, sf.AddrPC.Offset);
        write_function_name(os, process, sf.AddrPC.Offset);
        write_file_and_line(os, process, sf.AddrPC.Offset);

        os << "\n";
      }
    }
  }

  std::string get_exception_information(EXCEPTION_POINTERS & eps) {
    std::stringstream narrow_stream;
    int skip = 0;
    EXCEPTION_RECORD * er = eps.ExceptionRecord;
    switch (er->ExceptionCode) {
      case MSC_EXCEPTION_CODE: { // C++ exception
        UntypedException ue(eps);
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
    generate_stack_walk(narrow_stream, *(eps.ContextRecord), skip);
    return narrow_stream.str();
  }

  DWORD do_exception_filter(const tstring & exe_dir,
                            EXCEPTION_POINTERS & eps,
                            tstring & message) {
    std::string exception_information = get_exception_information(eps);

    tstringstream sstr;
    sstr << TBuffer(exception_information.c_str());

    std::ofstream error_log(NarrowBuffer((exe_dir + _T("\\err_log.txt")).c_str()));
    if (error_log) {
      error_log << exception_information;
      sstr << _T("\nThis message has been written to err_log.txt");
    }
  
    message = sstr.str();
    return EXCEPTION_EXECUTE_HANDLER;
  }

  DWORD exception_filter(const tstring & exe_dir, EXCEPTION_POINTERS & eps, tstring & message) {
    // in case of errors like a corrupted heap or stack or other error condition that
    //   prevents the stack trace from being built, just dump to the operating system
    //   exception handler and hope that the filter didn't mess up the program state
    //   sufficiently to make a crash dump useless
    __try {
      return do_exception_filter(exe_dir, eps, message);
    #pragma warning(suppress: 6320)
    } __except(EXCEPTION_EXECUTE_HANDLER) {
      return EXCEPTION_CONTINUE_SEARCH;
    }
  }

}