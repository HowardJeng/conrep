// settings.cpp
// implementation of settings object initialization

#include "settings.h"

#include <ShlObj.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#include "assert.h"
#include "exception.h"
#include "program_options.h"

#include <boost/filesystem.hpp>

using namespace boost::program_options;
using namespace boost::filesystem;

namespace console {
  const int MIN_COLUMNS = 40;

  #define DEFAULT_CFGFILE "conrep.cfg"

  #ifdef UNICODE
    #define tvalue wvalue
    #define DEFAULT_VALUE(x) default_value(_T(x), x)
    #define IMPLICIT_VALUE(x) implicit_value(_T(x), x)
  #else
    #define tvalue value
    #define DEFAULT_VALUE(x) default_value(x)
    #define IMPLICIT_VALUE(x) implicit_value(x)
  #endif

  void toupper(tstring & var_string) {
    std::transform(var_string.begin(), var_string.end(), var_string.begin(), _totupper);
  }

  bool get_bool(variables_map & vm, const char * name) {
    if (!vm.count(name)) return false;
    tstring var_string = vm[name].as<tstring>();
    toupper(var_string);
    if (var_string == _T("TRUE")) return true;
    return false;
  }

  void add_cmd_line_options(options_description & opt, tstring * config_file_name) {
    options_description cmd_line_desc("Command line only options");
    cmd_line_desc.add_options()
      ("cfgfile", tvalue(config_file_name)->DEFAULT_VALUE(DEFAULT_CFGFILE), "configuration file")
      ("adjust", "modify current conrep window")
      ("help", "display option descriptions")
    ;
    opt.add(cmd_line_desc);
  }

  void add_both_options(options_description & opt, Settings * s) {
    options_description both_desc("Command line and configuration file options");
    both_desc.add_options()
      ( "shell", 
        tvalue(s ? &(s->shell) : nullptr)->DEFAULT_VALUE("cmd.exe"), 
        "command shell to start" )
      ( "args", 
        tvalue(s ? &(s->shell_arguments) : nullptr )->DEFAULT_VALUE(""), 
        "arguments to pass to shell" )
      ( "font_name", 
        tvalue(s ? &(s->font_name) : nullptr)->DEFAULT_VALUE("Courier New"), 
        "* name of font to use" )
      ( "font_size", 
        tvalue(s ? &(s->font_size) : nullptr)->default_value(10), 
        "* size of font to use" )
      ( "rows", 
        tvalue(s ? &(s->rows) : nullptr)->default_value(24), 
        "* number of rows" )
      ( "columns", 
        tvalue(s ? &(s->columns) : nullptr)->default_value(80), 
        "* number of columns" )
      ( "maximize", 
        tvalue<tstring>()->DEFAULT_VALUE("false")->IMPLICIT_VALUE("true"), 
        "* fill the working area" )
      ( "extended_chars", 
        tvalue<tstring>()->DEFAULT_VALUE("false")->IMPLICIT_VALUE("true"), 
        "* use extended characters" )
      ( "intensify", 
        tvalue<tstring>()->DEFAULT_VALUE("false")->IMPLICIT_VALUE("true"), 
        "* intensify foreground colors" )
      ( "snap_distance", 
        tvalue(s ? &(s->snap_distance) : nullptr)->default_value(10), 
        "* distance to snap to work area" )
      ( "gutter_size", 
        tvalue(s ? &(s->gutter_size) : nullptr)->default_value(2), 
        "* size of inside border" )
      ( "z_order", 
        tvalue<tstring>()->DEFAULT_VALUE("normal"), 
        "* z order [top, bottom, normal]" )
      ( "active_pre_alpha", 
        tvalue(s ? &(s->active_pre_alpha) : nullptr)->default_value(0xA0), 
        "* pre-multiply alpha for active window" )
      ( "active_post_alpha", 
        tvalue(s ? &(s->active_post_alpha) : nullptr)->default_value(0xFF), 
        "* post-multiply alpha for active window" )
      ( "inactive_pre_alpha", 
        tvalue(s ? &(s->inactive_pre_alpha) : nullptr)->default_value(0xD0), 
        "* pre-multiply alpha for inactive window" )
      ( "inactive_post_alpha", 
        tvalue(s ? &(s->inactive_post_alpha) : nullptr)->default_value(0x50), 
        "* post-multiply alpha for inactive window" )
    ;
    opt.add(both_desc);
  }

  void add_hidden_options(options_description & opt) {
    options_description hidden_desc;
    hidden_desc.add_options()
      ( "execute_filter", 
        tvalue<tstring>()->DEFAULT_VALUE("true")->IMPLICIT_VALUE("true"), 
        "execute stack trace filter" )
    ;
    opt.add(hidden_desc);
  }

  void print_help(void) {
    options_description cmd_line_desc;
    add_cmd_line_options(cmd_line_desc, nullptr);
    add_both_options(cmd_line_desc, nullptr);
    std::cout << cmd_line_desc << std::endl;
    std::cout << "* settings that can be adjusted with --adjust" << std::endl;
  }

  CommandLineOptions::CommandLineOptions(LPCTSTR command_line) 
    : help(false),
      adjust(false)
  {
    const std::vector<tstring> args = split_winmain(command_line);
    options_description cmd_line_desc;
    add_cmd_line_options(cmd_line_desc, nullptr);

    variables_map vm;
    store(basic_command_line_parser<TCHAR>(args).options(cmd_line_desc).allow_unregistered().run(), vm);
    vm.notify();
    if (vm.count("help"))   help = true;
    if (vm.count("adjust")) adjust = true;
  }

  void parse_cmd_line(Settings & settings, variables_map & vm, LPCTSTR command_line, tstring * config_file_name) {
    const std::vector<tstring> args = split_winmain(command_line);

    options_description cmd_line_desc;

    add_cmd_line_options(cmd_line_desc, config_file_name);
    add_both_options(cmd_line_desc, &settings);
    add_hidden_options(cmd_line_desc);

    store(basic_command_line_parser<TCHAR>(args).options(cmd_line_desc).run(), vm);
    vm.notify();
    #define SCL(name) do { settings.scl_ ## name = !(vm[#name].defaulted()); } while (0)
    SCL(cfgfile);
    SCL(font_name);
    SCL(font_size);
    SCL(rows);
    SCL(columns);
    SCL(maximize);
    SCL(snap_distance);
    SCL(gutter_size);
    SCL(extended_chars);
    SCL(intensify);
    SCL(active_pre_alpha);
    SCL(active_post_alpha);
    SCL(inactive_pre_alpha);
    SCL(inactive_post_alpha);
    SCL(z_order);
    #undef SCL
  }

  void post_parse_fixups(variables_map & vm, Settings & settings) {
    settings.maximize = get_bool(vm, "maximize");
    if (settings.maximize) {
      settings.rows = -1;
      settings.columns = -1;
    } else {
      if (settings.columns < MIN_COLUMNS) settings.columns = MIN_COLUMNS;
    }
    settings.extended_chars = get_bool(vm, "extended_chars");
    settings.intensify = get_bool(vm, "intensify");
    settings.execute_filter = get_bool(vm, "execute_filter");

    tstring z_order_string = vm["z_order"].as<tstring>();
    toupper(z_order_string);
    if (z_order_string == _T("TOP")) {
      settings.z_order = Z_TOP;
    } else if (z_order_string == _T("BOTTOM")) {
      settings.z_order = Z_BOTTOM;
    } else {
      settings.z_order = Z_NORMAL;
    }

    settings.active_pre_alpha = std::min<unsigned int>(settings.active_pre_alpha, 
                                                       std::numeric_limits<unsigned char>::max());
    settings.active_post_alpha = std::min<unsigned int>(settings.active_post_alpha, 
                                                        std::numeric_limits<unsigned char>::max());
    settings.inactive_pre_alpha = std::min<unsigned int>(settings.inactive_pre_alpha, 
                                                         std::numeric_limits<unsigned char>::max());
    settings.inactive_post_alpha = std::min<unsigned int>(settings.inactive_post_alpha, 
                                                          std::numeric_limits<unsigned char>::max());

    if (settings.snap_distance < 0) settings.snap_distance = 0;
  }

  Settings::Settings(LPCTSTR command_line)
    : run_app(true)
  {
    variables_map vm;
    parse_cmd_line(*this, vm, command_line, nullptr);
    post_parse_fixups(vm, *this);
  }

  Settings::Settings(LPCTSTR command_line, LPCTSTR exe_directory, LPCTSTR working_directory)
    : run_app(true)
  {
    tstring config_file_name;
    variables_map vm;

    parse_cmd_line(*this, vm, command_line, &config_file_name);

    options_description both_desc;
    add_both_options(both_desc, this);
    add_hidden_options(both_desc);

    std::ifstream ifs;
    if (scl_cfgfile) {
      path config_file_path(config_file_name);
      if (config_file_path.is_absolute()) {
        ifs.open(config_file_name);
      } else {
        ASSERT(config_file_path.is_relative());
        ifs.open((path(working_directory) / config_file_name).c_str());
      }
      if (!ifs.is_open()) {
        tstringstream sstr;
        sstr << _T("Unable to open config file: ") << config_file_name;
        MessageBox(NULL, sstr.str().c_str(), _T("Command line error"), MB_OK);
      }
    }
    if (!ifs.is_open()) {
      TCHAR appdata[MAX_PATH];
      if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        ifs.open((path(appdata) / _T("Conrep") / _T(DEFAULT_CFGFILE)).c_str());
      }
      if (!ifs.is_open()) {
        ifs.open((path(exe_directory) / _T(DEFAULT_CFGFILE)).c_str());
      }
    }
    store(parse_config_file(ifs, both_desc), vm);
    vm.notify();

    post_parse_fixups(vm, *this);
  }
}