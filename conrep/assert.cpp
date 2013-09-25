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
