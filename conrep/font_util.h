#ifndef CONREP_FONT_UTIL_H
#define CONREP_FONT_UTIL_H

#include "windows.h"
#include "d3root.h"
#include "tchar.h"

namespace console {
  FontPtr create_font(DevicePtr device, const LOGFONT & lf);
  FontPtr create_font(DevicePtr device, const tstring & font_name, int font_size);
  Dimension get_char_dim(FontPtr font);
  void get_logfont(FontPtr font, LOGFONT * lf);
}

#endif