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

// exception.cpp
// implements various helper exception throwing functions

#include "exception.h"
#include <dxerr.h>
#include <sstream>
#include "file_util.h"

namespace console {
  void dx_exception(const char * msg, 
                    HRESULT hr, 
                    const char * file, 
                    const char * function, 
                    int line) {
    std::stringstream sstr;
    const char * error_description = DXGetErrorDescriptionA(hr);
    sstr << "DirectX error.\n"
         << msg
         << "\nError: " << hr << "(" << std::hex << hr << std::dec << "), "
         << error_description
         << "\nFile: " << get_leaf_from_path(file)
         << "\nFunction: " << function
         << "\nLine: " << line;
    throw std::runtime_error(sstr.str());
  }
  
  struct FormatLastError {
    FormatLastError(DWORD err) : err_code(err) {}
    DWORD err_code;
  };
  
  std::ostream & operator<<(std::ostream & lhs, const FormatLastError & rhs) {
    char * lpMsgBuf;

    if (FormatMessageA(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | 
          FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          rhs.err_code,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          reinterpret_cast<LPSTR>(&lpMsgBuf),
          0,
          NULL)) {
      try {
        lhs << " - " << lpMsgBuf;
      } catch (...) {
        LocalFree(lpMsgBuf);
      }
      LocalFree(lpMsgBuf);
    }
    return lhs;
  }
  
  void win_exception(const char * msg,
                     DWORD err,
                     const char * file, 
                     const char * function, 
                     int line) {
    std::stringstream sstr;
    sstr << "Win API error.\n"
         << msg
         << "\nError: " << err << FormatLastError(err)
         << "\nFile: " << get_leaf_from_path(file)
         << "\nFunction: " << function
         << "\nLine: " << line;
    throw std::runtime_error(sstr.str());
  }

  void gdiplus_exception(const char * msg, 
                         int status,
                         const char * file, 
                         const char * function, 
                         int line) {
    std::stringstream sstr;
    sstr << "GDI+ error.\n"
         << msg
         << "\nError code: " << status
         << "\nFile: " << get_leaf_from_path(file)
         << "\nFunction: " << function
         << "\nLine: " << line;
    throw std::runtime_error(sstr.str());
  }

  void misc_exception(const char * msg, 
                      const char * file, 
                      const char * function, 
                      int line) {
    std::stringstream sstr;
    sstr << msg
         << "\nFile: " << get_leaf_from_path(file)
         << "\nFunction: " << function
         << "\nLine: " << line;
    throw std::runtime_error(sstr.str());
  }
}