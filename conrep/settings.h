// Definition of the Settings structure
#ifndef CONREP_SETTINGS_H
#define CONREP_SETTINGS_H

#include "windows.h"
#include "tchar.h"

namespace console {
  enum ZOrder {
    Z_TOP,
    Z_BOTTOM,
    Z_NORMAL
  };

  void print_help(void);

  struct CommandLineOptions {
    CommandLineOptions(LPCTSTR command_line);

    bool help;
    bool adjust;
  };

  struct Settings {
    Settings(LPCTSTR command_line);
    Settings(LPCTSTR command_line, LPCTSTR exe_directory);
  
    bool run_app;
    
    tstring shell;
    tstring shell_arguments;
    
    tstring font_name;
    int     font_size;
    
    int rows;
    int columns;
    bool maximize;
    
    int snap_distance;
    int gutter_size;
    
    bool extended_chars;
    bool intensify;
    bool execute_filter;
    
    ZOrder z_order;

    unsigned int active_pre_alpha;
    unsigned int active_post_alpha;
    unsigned int inactive_pre_alpha;
    unsigned int inactive_post_alpha;

    bool scl_font_name;
    bool scl_font_size;
    
    bool scl_rows;
    bool scl_columns;
    bool scl_maximize;
    
    bool scl_snap_distance;
    bool scl_gutter_size;
    
    bool scl_extended_chars;
    bool scl_intensify;

    bool scl_active_pre_alpha;
    bool scl_active_post_alpha;
    bool scl_inactive_pre_alpha;
    bool scl_inactive_post_alpha;

    bool scl_z_order;
  };
}

#endif