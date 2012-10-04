#include "assert.h"

#include <sstream>
#include "except_handle.h"
#include "file_util.h"

namespace console {
  #ifdef NDEBUG
    const bool assert_halt_first = false;
  #else
    const bool assert_halt_first = true;
  #endif
  
  void assert_impl(const char * expression, const char * file, const char * function, unsigned line) {
    std::stringstream sstr;
    sstr << "Assertion failed: " << expression
         << "\nFile: " << get_leaf_from_path(file)
         << "\nFunction: " << function
         << "\nLine: " << line;

    ULONG_PTR assert_message = reinterpret_cast<ULONG_PTR>(_strdup(sstr.str().c_str()));
    RaiseException(ASSERT_EXCEPTION_CODE, EXCEPTION_NONCONTINUABLE, 1, &assert_message);
  }
}
