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

  struct Settings {
    Settings(DWORD process_id, LPCTSTR command_line, LPCTSTR exe_directory);
    
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
  };
}

#endif