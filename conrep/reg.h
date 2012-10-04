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