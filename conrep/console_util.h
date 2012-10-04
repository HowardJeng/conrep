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