// reg.cpp
// implements helper functions for registry access
#include "reg.h"

#include <WinInet.h> // required for shlobj.h
#include <ShlObj.h>

#include "assert.h"
#include "exception.h"
#include "lexical_cast.h"

namespace console {
  tstring get_string_value(CRegKey & key, LPCTSTR value_name) {
    const int BUFFER_SIZE = 1024;
    TCHAR tmp[BUFFER_SIZE] = {};
    ULONG size = BUFFER_SIZE;

    LONG ret_val = key.QueryStringValue(value_name, tmp, &size);
    ASSERT(size <= BUFFER_SIZE);
    
    if (ret_val == ERROR_SUCCESS) {
      return tstring(tmp, size - 1);
    } else if (ret_val == ERROR_MORE_DATA) {
      std::vector<TCHAR> buffer(BUFFER_SIZE);
      do {
        buffer.resize(buffer.size() * 2);
        ASSERT(buffer.size() < 0x100000000ull);
        size = static_cast<ULONG>(buffer.size());
        ret_val = key.QueryStringValue(value_name, &buffer[0], &size);
      } while (ret_val == ERROR_MORE_DATA);
      if (ret_val == ERROR_SUCCESS) return tstring(&buffer[0], size - 1);
    }
    
    WIN_EXCEPT2("Failed call to QueryStringValue(). ", ret_val);
  }

  WallpaperInfo get_wallpaper_info(void) {
    CComPtr<IActiveDesktop> desktop;
    HRESULT hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (void**)&desktop);
    if (FAILED(hr)) {
      DX_EXCEPT("Error creating IActiveDesktop interface. ", hr);
    }

    WCHAR name[MAX_PATH];
    hr = desktop->GetWallpaper(name, MAX_PATH, 0);
    if (FAILED(hr)) {
      DX_EXCEPT("Error getting wallpaper name. ", hr);
    }
    WALLPAPEROPT opt = { sizeof(WALLPAPEROPT) };
    hr = desktop->GetWallpaperOptions(&opt, 0);
    if (FAILED(hr)) {
      DX_EXCEPT("Error getting wallpaper style. ", hr);
    }

    WallpaperInfo wi = {
      TBuffer(name),
      static_cast<WallpaperStyle>(opt.dwStyle)
    };

    return wi;
  }

  bool operator==(const WallpaperInfo & lhs, const WallpaperInfo & rhs) {
    return (lhs.style == rhs.style) && (lhs.wallpaper_name == rhs.wallpaper_name);
  }
  bool operator!=(const WallpaperInfo & lhs, const WallpaperInfo & rhs) {
    return (lhs.style != rhs.style) || (lhs.wallpaper_name != rhs.wallpaper_name);
  }
}