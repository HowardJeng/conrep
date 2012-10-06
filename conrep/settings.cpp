// settings.cpp
// implementation of settings object initialization

#include "settings.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4512)
#include <boost/program_options.hpp>
#pragma warning(pop)

using namespace boost::program_options;

namespace console {
  #ifdef UNICODE
    #define tvalue wvalue
    #define DEFAULT_VALUE(x) default_value(_T(x), x)
  #else
    #define tvalue value
    #define DEFAULT_VALUE(x) default_value(x)
  #endif

  void toupper(tstring & var_string) {
    std::transform(var_string.begin(), var_string.end(), var_string.begin(), _totupper);
  }

  bool get_bool(variables_map & vm, const char * name) {
    tstring var_string = vm[name].as<tstring>();
    toupper(var_string);
    if (var_string == _T("TRUE")) return true;
    return false;
  }

  Settings::Settings(DWORD process_id, LPCTSTR command_line, LPCTSTR exe_directory)
    : run_app(true)
  {
    tstring config_file_name;
    options_description config_file_desc;
    config_file_desc.add_options()
      ("cfgfile", tvalue(&config_file_name)->DEFAULT_VALUE("conrep.cfg"), "configuration file")
      ("help", "display option descriptions")
    ;

    options_description main_options;
    main_options.add_options()
      ("shell", tvalue(&shell)->DEFAULT_VALUE("cmd.exe"), "command shell to start")
      ("args", tvalue(&shell_arguments)->DEFAULT_VALUE(""), "arguments to pass to shell")
      ("font_name", tvalue(&font_name)->DEFAULT_VALUE("Courier New"), "name of font to use")
      ("font_size", tvalue(&font_size)->default_value(10), "size of font to use")
      ("rows", tvalue(&rows)->default_value(24), "number of rows")
      ("columns", tvalue(&columns)->default_value(80), "number of columns")
      ("maximize", tvalue<tstring>()->DEFAULT_VALUE("false"), "fill the working area")
      ("extended_chars", tvalue<tstring>()->DEFAULT_VALUE("false"), "use extended characters")
      ("intensify", tvalue<tstring>()->DEFAULT_VALUE("false"), "intensify foreground colors")
      ("execute_filter", tvalue<tstring>()->DEFAULT_VALUE("true"), "execute stack trace filter")
      ("snap_distance", tvalue(&snap_distance)->default_value(10), "distance to snap to work area")
      ("gutter_size", tvalue(&gutter_size)->default_value(2), "size of inside border")
      ("z_order", tvalue<tstring>()->DEFAULT_VALUE("normal"), "z order [top, bottom, normal]")
      ("active_pre_alpha", tvalue(&active_pre_alpha)->default_value(0xA0), "pre-multiply alpha for active window")
      ("active_post_alpha", tvalue(&active_post_alpha)->default_value(0xFF), "post-multiply alpha for active window")
      ("inactive_pre_alpha", tvalue(&inactive_pre_alpha)->default_value(0xD0), "pre-multiply alpha for inactive window")
      ("inactive_post_alpha", tvalue(&inactive_post_alpha)->default_value(0x50), "post-multiply alpha for inactive window")
    ;

    const std::vector<tstring> args = split_winmain(command_line);

    variables_map vm;
    options_description cmd_line_options;
    cmd_line_options.add(config_file_desc).add(main_options);
    store(basic_command_line_parser<TCHAR>(args).options(cmd_line_options).allow_unregistered().run(), vm);
    vm.notify();
    if ((config_file_name.find(_T('\\')) == tstring::npos) &&
        (config_file_name.find(_T('/')) == tstring::npos)) {
      config_file_name = tstring(exe_directory) + _T("\\") + config_file_name;
    }

    if (vm.count("help")) {      
      if (AttachConsole(process_id)) {
        freopen("CONOUT$", "w", stdout);
        std::cout << cmd_line_options << std::endl;
        FreeConsole();
      }
      run_app = false;
      return;
    }

    std::ifstream ifs(config_file_name.c_str());
    store(parse_config_file(ifs, main_options), vm);
    vm.notify();

    maximize = get_bool(vm, "maximize");
    if (maximize) {
      rows = -1;
      columns = -1;
    }
    extended_chars = get_bool(vm, "extended_chars");
    intensify = get_bool(vm, "intensify");
    execute_filter = get_bool(vm, "execute_filter");

    tstring z_order_string = vm["z_order"].as<tstring>();
    toupper(z_order_string);
    if (z_order_string == _T("TOP")) {
      z_order = Z_TOP;
    } else if (z_order_string == _T("BOTTOM")) {
      z_order = Z_BOTTOM;
    } else {
      z_order = Z_NORMAL;
    }

    active_pre_alpha = std::min<unsigned int>(active_pre_alpha, std::numeric_limits<unsigned char>::max());
    active_post_alpha = std::min<unsigned int>(active_post_alpha, std::numeric_limits<unsigned char>::max());
    inactive_pre_alpha = std::min<unsigned int>(inactive_pre_alpha, std::numeric_limits<unsigned char>::max());
    inactive_post_alpha = std::min<unsigned int>(inactive_post_alpha, std::numeric_limits<unsigned char>::max());
  }
}