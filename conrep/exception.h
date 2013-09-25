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

#ifndef CONREP_EXCEPTION_H
#define CONREP_EXCEPTION_H

#include "windows.h"
#include <iosfwd>

#define DX_EXCEPT(msg, hr)      dx_exception(msg, hr, __FILE__, __FUNCTION__, __LINE__)
#define WIN_EXCEPT(msg)         win_exception(msg, GetLastError(), __FILE__, __FUNCTION__, __LINE__)
#define WIN_EXCEPT2(msg, err)   win_exception(msg, err, __FILE__, __FUNCTION__, __LINE__)
#define GDIPLUS_EXCEPT(msg, st) gdiplus_exception(msg, st, __FILE__, __FUNCTION__, __LINE__)
#define MISC_EXCEPT(msg)        misc_exception(msg, __FILE__, __FUNCTION__, __LINE__)

struct ExceptionTypeInfo;

namespace console {
  __declspec(noreturn, noinline) void dx_exception(const char * msg, HRESULT hr, const char * file, const char * function, int line);
  __declspec(noreturn, noinline) void win_exception(const char * msg, DWORD err, const char * file, const char * function, int line);
  __declspec(noreturn, noinline) void gdiplus_exception(const char * msg, int status, const char * file, const char * function, int line);
  __declspec(noreturn, noinline) void misc_exception(const char * msg, const char * file, const char * function, int line);
}

#endif