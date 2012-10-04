// console_util.cpp
// implements utility functions for dealing with the created console window:
//   mostly calculation of various positioning and dimension information

#include "console_util.h"

#include <cstdlib>
#include "dimension.h"
#include "exception.h"

namespace console {
  void snap_window(RECT & window, const RECT & bounds, int snap_distance) {
    int delta_left   = window.left   - bounds.left;
    int delta_right  = window.right  - bounds.right;
    int delta_top    = window.top    - bounds.top;
    int delta_bottom = window.bottom - bounds.bottom;
    
    if ((std::abs(delta_left) < snap_distance) &&
        (std::abs(delta_left) <= std::abs(delta_right))) {
      window.left -= delta_left;
      window.right -= delta_left;
    }
    if ((std::abs(delta_right) < snap_distance) &&
        (std::abs(delta_right) < std::abs(delta_left))) {
      window.left -= delta_right;
      window.right -= delta_right;
    }
    if ((std::abs(delta_top) < snap_distance) &&
        (std::abs(delta_top) <= std::abs(delta_bottom))) {
      window.top -= delta_top;
      window.bottom -= delta_top;
    }
    if ((std::abs(delta_bottom) < snap_distance) &&
        (std::abs(delta_bottom) < std::abs(delta_top))) {
      window.top -= delta_bottom;
      window.bottom -= delta_bottom;
    }
  }

  Dimension calc_client_size(Dimension char_dim, Dimension console_dim, int gutter_size) {
    return Dimension(console_dim.width * char_dim.width + 2 * gutter_size,
                     console_dim.height * char_dim.height + 2 * gutter_size);
  }
  
  Dimension calc_window_size(Dimension client_dim, int scroll_width, int window_style) {
    RECT r = { 0, 0, client_dim.width - 1, client_dim.height - 1 };
    if (!AdjustWindowRect(&r, window_style, false))
      WIN_EXCEPT("Failure in call to AdjustWindowRect(). ");
    return Dimension(r.right - r.left + 1 + scroll_width,
                     r.bottom - r.top + 1);
  }
  
  RECT get_work_area(void) {
    RECT ret_val = {};
    if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &ret_val, 0))
      WIN_EXCEPT("Failure in SystemParamterInfo() call with SPI_GETWORKAREA. ");
    return ret_val;
  }
  
  Dimension get_max_window_dim(const RECT & work_area) {
    return Dimension(work_area.right - work_area.left + 1,
                     work_area.bottom - work_area.top + 1);
  }
  
  Dimension get_client_dim(Dimension max_window_dim,
                           int scrollbar_width,
                           int window_style) {
    const int BASE_DIM = 100;
    RECT r = { 0, 0, BASE_DIM - 1, BASE_DIM - 1 };
    AdjustWindowRect(&r, window_style, false);
    int x_offset = r.right - r.left + 1 - BASE_DIM + scrollbar_width;
    int y_offset = r.bottom - r.top + 1 - BASE_DIM;

    int max_client_x = max_window_dim.width - x_offset;
    int max_client_y = max_window_dim.height - y_offset;
    return Dimension(max_client_x, max_client_y);
  }
  
  Dimension get_max_usable_client_dim(Dimension max_window_dim, 
                                      int gutter_size,
                                      int scrollbar_width,
                                      int window_style) {
    Dimension d = get_client_dim(max_window_dim, scrollbar_width, window_style);
    return Dimension(d.width - 2 * gutter_size, d.height - 2 * gutter_size);
  }

}