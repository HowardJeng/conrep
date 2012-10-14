#ifndef CONREP_EXCEPT_HANDLE_H
#define CONREP_EXCEPT_HANDLE_H

#include "windows.h"
#include <iosfwd>
#include "tchar.h"

const DWORD ASSERT_EXCEPTION_CODE = 0xE0417372;
const DWORD MSC_EXCEPTION_CODE = 0xE06D7363;
const int EXIT_ABNORMAL_TERMINATION = -10;

struct ExceptionTypeInfo;

namespace console {
  void generate_stack_walk(std::ostream & os, CONTEXT ctx, int skip = 0);
  std::string get_exception_information(EXCEPTION_POINTERS & eps);
  DWORD exception_filter(const tstring & exe_dir, EXCEPTION_POINTERS & eps, tstring & message);

  #ifdef _M_IX86
    struct UntypedException {
      UntypedException(EXCEPTION_POINTERS & eps)
        : exception_object(reinterpret_cast<void *>((eps.ExceptionRecord->ExceptionInformation)[1])),
          type_array(reinterpret_cast<_ThrowInfo *>((eps.ExceptionRecord->ExceptionInformation)[2])->pCatchableTypeArray)
      {}
      void * exception_object;
      _CatchableTypeArray * type_array;
    };
     
  #elif defined(_M_X64) && (_MSC_VER >= 1400)
    struct UntypedException {
      UntypedException(EXCEPTION_POINTERS & eps)
        : exception_pointers(&eps)
      {}

      EXCEPTION_POINTERS * exception_pointers;
    };
  #else
    #error Unsupported platform
  #endif 

  void * exception_cast_worker(const UntypedException & e, const type_info & ti);
  void get_exception_types(std::ostream & os, const UntypedException & e);

  template <typename T>
  T * exception_cast(const UntypedException & e) {
    const std::type_info & ti = typeid(T);
    return reinterpret_cast<T *>(exception_cast_worker(e, ti));
  }
}

#endif