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
    Settings(LPCTSTR command_line, LPCTSTR exe_directory, LPCTSTR working_directory);
  
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

    bool scl_cfgfile;

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