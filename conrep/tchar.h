// Wide/narrow string types
#ifndef CONREP_TCHAR_H
#define CONREP_TCHAR_H

#include <tchar.h>
#include <iosfwd>
#include <string>
#include <vector>

namespace console {
  typedef std::basic_string<TCHAR> tstring;
  typedef std::basic_ifstream<TCHAR> tifstream;
  // typedef possible through <iosfwd>. If tstringstream instantiated then need to
  //   include <sstream> in addition to "tchar.h"
  typedef std::basic_stringstream<TCHAR> tstringstream;
  typedef std::basic_ostringstream<TCHAR> tostringstream;
  
  class WideBuffer {
    public:
      WideBuffer(const char * narrow_str) : buffer_(mbstowcs(0, narrow_str, 0) + 1) {
        mbstowcs(&buffer_[0], narrow_str, buffer_.size() - 1);
      }
      WideBuffer(const wchar_t * wide_str) : buffer_(wcslen(wide_str) + 1) {
        wcscpy(&buffer_[0], wide_str);
      }
      
      operator const wchar_t *() const {
        return &buffer_[0];
      }
    private:
      std::vector<wchar_t> buffer_;
  };
  
  template <typename Traits>
  std::basic_ostream<wchar_t, Traits> & operator<<(std::basic_ostream<wchar_t, Traits> & lhs,
                                                   const WideBuffer & rhs) {
    return lhs << rhs.operator const wchar_t *();
  }
  
  class NarrowBuffer {
    public:
      NarrowBuffer(const char * narrow_str) : buffer_(strlen(narrow_str) + 1) {
        strcpy(&buffer_[0], narrow_str);
      }
      NarrowBuffer(const wchar_t * wide_str) : buffer_(wcstombs(0, wide_str, 0) + 1) {
        wcstombs(&buffer_[0], wide_str, buffer_.size() - 1);
      }
      operator const char *() const {
        return &buffer_[0];
      }
    private:
      std::vector<char> buffer_;
  };
  template <typename Traits>
  std::basic_ostream<char, Traits> & operator<<(std::basic_ostream<char, Traits> & lhs,
                                                const NarrowBuffer & rhs) {
    return lhs << rhs.operator const char *();
  }
  
  #ifdef _UNICODE
    #define TBuffer WideBuffer
  #else
    #define TBuffer NarrowBuffer
  #endif
}

#endif