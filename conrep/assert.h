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

#ifndef CONREP_ASSERT_H
#define CONREP_ASSERT_H

#ifdef ASSERT_DIABLED
  #define ASSERT(exp) do { (void)sizeof(exp); __assume(exp); } while (0)
#else
  #define ASSERT(exp)                                                   \
    do {                                                                \
      if (!(exp)) {                                                     \
        if (::console::assert_halt_first) { __debugbreak(); }           \
        ::console::assert_impl(#exp, __FILE__, __FUNCTION__, __LINE__); \
      }                                                                 \
      __assume(exp);                                                    \
    } while (0)
#endif

namespace console {
  extern const bool assert_halt_first;

  __declspec(noreturn) void assert_impl(const char * expression, const char * file, const char * function, unsigned line);
}

#endif