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

#ifndef CONREP_FILE_UTIL_H
#define CONREP_FILE_UTIL_H

#include <string>
#include "tchar.h"
#include "windows.h"

namespace console {
  std::string  get_leaf_from_path(const std::string  & path);
  std::wstring get_leaf_from_path(const std::wstring & path);

  std::string  get_branch_from_path(const std::string  & path);
  std::wstring get_branch_from_path(const std::wstring & path);
  
  std::string  get_module_path_a(HMODULE module = 0);
  std::wstring get_module_path_w(HMODULE module = 0);

  #ifdef _UNICODE
    #define get_module_path get_module_path_w
  #else
    #define get_module_path get_module_path_a
  #endif
 
  FILETIME get_modify_time(const tstring & file_name);
}

#endif