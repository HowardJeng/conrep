#include "file_util.h"

#include <sstream>

#include "assert.h"
#include "atl.h"
#include "exception.h"

namespace console {
  std::string get_leaf_from_path(const std::string & path) {
    std::string file_name(path);
    std::string::size_type pos = file_name.find_last_of('\\');
    if (pos != std::string::npos) {
      file_name.erase(0, pos + 1);
    }
    return file_name;
  }

  std::wstring get_leaf_from_path(const std::wstring & path) {
    std::wstring file_name(path);
    std::wstring::size_type pos = file_name.find_last_of(L'\\');
    if (pos != std::wstring::npos) {
      file_name.erase(0, pos + 1);
    }
    return file_name;
  }
  
  std::string get_branch_from_path(const std::string & path) {
    std::string directory(path);
    directory.erase(directory.find_last_of('\\'), std::string::npos);
    return directory;
  }

  std::wstring get_branch_from_path(const std::wstring & path) {
    std::wstring directory(path);
    directory.erase(directory.find_last_of(L'\\'), std::wstring::npos);
    return directory;
  }

  std::string get_module_path_a(HMODULE module) {
	  char path_name[MAX_PATH] = {};
    DWORD size = GetModuleFileNameA(module, path_name, MAX_PATH);
    return std::string(path_name, size);
  }

  std::wstring get_module_path_w(HMODULE module) {
	  wchar_t path_name[MAX_PATH] = {};
    DWORD size = GetModuleFileNameW(module, path_name, MAX_PATH);
    return std::wstring(path_name, size);
  }

  FILETIME get_modify_time(const tstring & file_name) {
    ASSERT(!file_name.empty());
    ASSERT(file_name != _T(""));
    CHandle h(CreateFile(file_name.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0));
    if (h == INVALID_HANDLE_VALUE) {
      std::stringstream sstr;
      sstr << "Failed call to CreateFile() opening \"" << NarrowBuffer(file_name.c_str()) << "\"";
      WIN_EXCEPT(sstr.str().c_str());
    }
    FILETIME ret_val;
    if (!GetFileTime(h, 0, 0, &ret_val))
      WIN_EXCEPT("Failed call to GetFileTime(). ");
    return ret_val;
  }
}
