// settings.cpp
// implementation of settings object initialization

#include "settings.h"

#include <algorithm>

#include "pywrapper.h"

namespace console {
  bool is_slash_or_quote(TCHAR c) {
    switch (c) {
      case _T('\\'):
      case _T('"'):
        return true;
      default:
        return false;
    }
  }
    
  class SlashQuoteSpace : std::unary_function<bool, TCHAR> {
    public:
      SlashQuoteSpace(const std::ctype<TCHAR> & ct) : ct_(ct) {}
      bool operator()(TCHAR c) {
        switch (c) {
          case _T('\\'):
          case _T('"'):
            return true;
          default:
            return ct_.is(std::ctype_base::space, c);
        }
      }
    private:
      SlashQuoteSpace & operator=(const SlashQuoteSpace &);
      const std::ctype<TCHAR> & ct_;
  };
  
  // splits command line into argument vector. does not handle wildcard expansion
  //   attempts to follow character interpretations as specified in
  //   CommandLineToArgvW() MSDN entry. Does not use CommandLineToArgvW() as that
  //   function implicitly adds the module name to the command line (and then
  //   procedes to parse without regards to spaces).
  std::vector<tstring> split_winmain(LPCTSTR input) {
    size_t len = _tcslen(input);
      
    const TCHAR * i = input;
    const TCHAR * e = input + len;

    std::locale loc;
    const std::ctype<TCHAR> & ct = std::use_facet<std::ctype<TCHAR> >(loc);
    i = ct.scan_not(std::ctype_base::space, i, e);
      
    tstring current;
    bool inside_quoted = false;
    int backslash_count = 0;

    std::vector<tstring> result;
    while (i != e) {
      if (*i == _T('"')) {
        // '"' preceded by even number (n) of backslashes generates
        // n/2 backslashes and is a quoted block delimiter
        current.append(backslash_count / 2, _T('\\'));
        if (backslash_count % 2 == 0) {
          inside_quoted = !inside_quoted;
        } else {
          current += _T('"');
        }
        backslash_count = 0;
        ++i;
      } else if (*i == _T('\\')) {
        ++backslash_count;
        ++i;
      } else {
        // Not quote or backslash. All accumulated backslashes should be
        // added
        if (backslash_count) {
          current.append(backslash_count, _T('\\'));
          backslash_count = 0;
        }
        if (ct.is(std::ctype_base::space, *i) && !inside_quoted) {
          // Space outside quoted section terminate the current argument
          result.push_back(current);
          current.resize(0);
          i = ct.scan_not(std::ctype_base::space, i, e);
        } else if (inside_quoted) {
          const TCHAR * ptr = std::find_if(i + 1, e, is_slash_or_quote);
          current.append(i, ptr);
          i = ptr;
        } else {
          const TCHAR * ptr = std::find_if(i + 1, e, SlashQuoteSpace(ct));
          current.append(i, ptr);
          i = ptr;
        }
      }
    }

    // If we have trailing backslashes, add them
    current.append(backslash_count, '\\');

    // If we have non-empty 'current' or we're still in quoted
    // section (even if 'current' is empty), add the last token.
    if (!current.empty() || inside_quoted)
      result.push_back(current);        

    return result;
  }

  Settings::Settings(LPCTSTR command_line, LPCTSTR exe_directory)
    : run_app(true)
  {
    using namespace std;
    using namespace boost::python;
    
    vector<tstring> args = split_winmain(command_line);
    PyWrapper wrap;
    object arg_list = list();
    for (vector<tstring>::iterator i = args.begin();
         i != args.end();
         ++i) {
      arg_list.attr("append")(*i);
    }
    
    object config_file;
    { tstring file_name = tstring(exe_directory) + _T("\\conrep.cfg");
      object builtins = wrap["__builtins__"];
      object open = builtins.attr("open");
      config_file = open(file_name);
    }

    object settings_mod = wrap.import("settings");
    object parse_settings = settings_mod["parse_settings"];
    object result = parse_settings(arg_list, config_file);
    config_file.attr("close")();
    
    tstring err = extract<tstring>(result.attr("err"));
    if (err != _T("")) {
      MessageBox(0, err.c_str(), _T("Config error"), MB_OK);
      run_app = false;
      return;
    }
    
    shell           = extract<tstring>(result.attr("shell"));
    shell_arguments = extract<tstring>(result.attr("args"));
    
    font_name = extract<tstring>(result.attr("font_name"));
    font_size = extract<int>(result.attr("font_size"));
    
    tstring max_string = extract<tstring>(result.attr("maximize"));
    if (max_string == _T("TRUE")) {
      maximize = true;
      rows = -1;
      columns = -1;
    } else {
      maximize = false;
      rows = extract<int>(result.attr("rows"));
      columns = extract<int>(result.attr("columns"));
    }
    
    snap_distance = extract<int>(result.attr("snap_distance"));
    gutter_size = extract<int>(result.attr("gutter_size"));
    
    tstring extended_chars_string = extract<tstring>(result.attr("extended_chars"));
    extended_chars = (extended_chars_string == _T("TRUE"));
    
    tstring intensify_string = extract<tstring>(result.attr("intensify"));
    intensify = (intensify_string == _T("TRUE"));
    
    tstring execute_filter_string = extract<tstring>(result.attr("execute_filter"));
    execute_filter = (execute_filter_string == _T("TRUE"));
    
    tstring z_order_string = extract<tstring>(result.attr("zorder"));
    if (z_order_string == _T("TOP")) {
      z_order = Z_TOP;
    } else if (z_order_string == _T("BOTTOM")) {
      z_order = Z_BOTTOM;
    } else {
      z_order = Z_NORMAL;
    }
  }
}