#include "except_handle.h"

#include <dbghelp.h>
#include "file_util.h"

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


}