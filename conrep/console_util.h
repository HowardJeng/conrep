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

// Utility functions for manipulating the console replacement window.

#ifndef CONREP_CONSOLE_UTIL_H
#define CONREP_CONSOLE_UTIL_H

#include "windows.h"

namespace console {
  struct Dimension;

  void snap_window(RECT & window, const RECT & bounds, int snap_distance);
  Dimension calc_client_size(Dimension char_dim, Dimension console_dim, int gutter_size);
  Dimension calc_window_size(Dimension client_dim, int scroll_width,int window_style);
  RECT get_work_area(void);
  Dimension get_max_window_dim(const RECT & work_area);
  Dimension get_client_dim(Dimension max_window_dim, int scrollbar_width, int window_style);
  Dimension get_max_usable_client_dim(Dimension max_window_dim, int gutter_size, int scrollbar_width, int window_style);
}

#endif