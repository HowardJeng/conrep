#ifndef CONREP_FONT_UTIL_H
#define CONREP_FONT_UTIL_H

#include "windows.h"
#include "d3root.h"
#include "tchar.h"

namespace console {
  FontPtr create_font(DevicePtr device, const LOGFONT & lf);

  const int POINT_SIZE_SCALE = 10;
  // font_size is in units of tenths of point size. i.e. 100 is a 10 point font
  FontPtr create_font(DevicePtr device, const tstring & font_name, int font_size);

  Dimension get_char_dim(FontPtr font);
  void get_logfont(FontPtr font, LOGFONT * lf);
}

#endif