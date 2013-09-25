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

// Helper functions for ATL::CRegKey

#ifndef CONREP_REG_H
#define CONREP_REG_H

#include "atl.h"
#include "tchar.h"

namespace console {
  tstring get_string_value(ATL::CRegKey & key, LPCTSTR value_name);
  
  enum WallpaperStyle {
    CENTER,
    TILE,
    STRETCH,
    ASPECT_PAD,  // not implemented
    ASPECT_CROP  // not implemented
  };

  struct WallpaperInfo {
    tstring wallpaper_name;
    WallpaperStyle style;
  };

  bool operator==(const WallpaperInfo & lhs, const WallpaperInfo & rhs);
  bool operator!=(const WallpaperInfo & lhs, const WallpaperInfo & rhs);
  
  WallpaperInfo get_wallpaper_info(void);
}

#endif