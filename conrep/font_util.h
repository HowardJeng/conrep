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