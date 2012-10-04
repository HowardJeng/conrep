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