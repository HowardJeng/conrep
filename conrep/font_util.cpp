// font_util.cpp
// implements helper functions dealing with fonts
// TODO: make QUALITY a setting rather than hard coded value

#include "font_util.h"

#include <sstream>
#include "assert.h"
#include "dimension.h"
#include "exception.h"
#include "gdiplus.h"

using namespace ATL;
using namespace Gdiplus;

namespace console {
  //const int QUALITY = CLEARTYPE_QUALITY;
  const int QUALITY = ANTIALIASED_QUALITY;
  
  // Uses GDI+ to fill the LOGFONT structure. This is computationally expensive
  //   but easy to get right. However it doesn't handle some fonts like Terminal.
  //   font_size is in units of tenths of point size. i.e. 100 is a 10 point font
  bool get_logfont(const tstring & font_name, int font_size, LOGFONT * lf) {
    Bitmap b(1, 1);
    Graphics g(&b);

    FontFamily font_family(WideBuffer(font_name.c_str()));
    const FontFamily * ff = &font_family;
    if (!font_family.IsAvailable()) 
      return false;
    Gdiplus::Font font(ff, static_cast<REAL>(font_size) / POINT_SIZE_SCALE);
    if (!font.IsAvailable()) 
      return false;
      
    #ifdef UNICODE
      font.GetLogFontW(&g, lf);
    #else
      font.GetLogFontA(&g, lf);
    #endif
    return true;
  }

  // sets the bool to true if any of the enumerated fonts is fixed width. 
  int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe,
                                  NEWTEXTMETRICEX *,
                                  DWORD,
                                  LPARAM lParam) {
    if (lpelfe->elfLogFont.lfPitchAndFamily & FIXED_PITCH) {
      *reinterpret_cast<bool *>(lParam) = true;
      return 0;
    }
    return 1;
  }

  bool is_fixed_width(const tstring & font_name) {
    if (font_name.length() > 31) return false;
  
    HDC dc = GetDC(NULL);
    if (!dc) WIN_EXCEPT("Failed call to GetDC(NULL).");
    
    LOGFONT lf = {};
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;
    // already checked length
    TCHAR * ptr = std::copy(font_name.begin(), font_name.end(), lf.lfFaceName); 
    ASSERT(ptr <= (lf.lfFaceName + 32)); (void)ptr;
    
    bool is = false;
    EnumFontFamiliesEx(dc, 
                       &lf, 
                       reinterpret_cast<FONTENUMPROC>(EnumFontFamExProc), 
                       reinterpret_cast<LPARAM>(&is), 
                       0);
    
    ReleaseDC(NULL, dc);
    return is;
  }
  
  FontPtr create_font(DevicePtr device, const LOGFONT & lf) {
    FontPtr font;
    HRESULT hr = D3DXCreateFont(device,
                                lf.lfHeight,
                                lf.lfWidth,
                                lf.lfWeight,
                                D3DX_DEFAULT,
                                lf.lfItalic,
                                lf.lfCharSet,
                                lf.lfOutPrecision,
                                QUALITY,
                                lf.lfPitchAndFamily,
                                lf.lfFaceName,
                                &font);
    if (FAILED(hr)) DX_EXCEPT("Failed call to D3DXCreateFont(). ", hr);
    return font;
  }

  // font_size is in units of tenths of point size. i.e. 100 is a 10 point font
  FontPtr create_font(DevicePtr device, const tstring & font_name, int font_size) {
    HDC dc = GetDC(NULL);
    if (!dc) WIN_EXCEPT("Failed call to GetDC(NULL).");
    int log_pixels_y = GetDeviceCaps(dc, LOGPIXELSY);
    ReleaseDC(NULL, dc);
  
    FontPtr font;
    HRESULT hr = E_FAIL;
    if (is_fixed_width(font_name)) {
      LOGFONT lf;
      // Try filling the logfont with GDI+ first since it gives more natural font
      //   proportions than passing 0 as the width. However since GDI+ doesn't handle
      //   some fonts like Terminal properly, if GDI+ fails, try creating the font
      //   directly.
      if (get_logfont(font_name, font_size, &lf))
        return create_font(device, lf);
      hr = D3DXCreateFont(device,
                          -MulDiv(font_size, log_pixels_y, 72 * POINT_SIZE_SCALE),
                          0,
                          FW_NORMAL,
                          D3DX_DEFAULT,
                          FALSE,
                          DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS,
                          QUALITY,
                          FIXED_PITCH | FF_DONTCARE,
                          font_name.c_str(),
                          &font);
    }
    if (FAILED(hr)) {
      tstringstream sstr;
      sstr << _T("Unable to use the font ") << font_name << ".";
      MessageBox(NULL, sstr.str().c_str(), _T("Font error"), MB_OK);
      hr = D3DXCreateFont(device,
                          -MulDiv(font_size, log_pixels_y, 72 * POINT_SIZE_SCALE),
                          0,
                          FW_NORMAL,
                          D3DX_DEFAULT,
                          FALSE,
                          DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS,
                          QUALITY,
                          FIXED_PITCH | FF_DONTCARE,
                          _T("Lucida Console"),
                          &font);
      if (FAILED(hr)) DX_EXCEPT("Failed call to D3DXCreateFont(). ", hr);
    }
    return font;
  }
  
  Dimension get_char_dim(FontPtr font) {
    TEXTMETRIC tm;
    if (!font->GetTextMetrics(&tm)) MISC_EXCEPT("Failed call to ID3DXFont::GetTextMetrics(). ");
    return Dimension(tm.tmAveCharWidth, tm.tmHeight);
  }

  void get_logfont(FontPtr font, LOGFONT * lf) {
    D3DXFONT_DESC fd;
    HRESULT hr = font->GetDesc(&fd);
    if (FAILED(hr)) DX_EXCEPT("Failed call to ID3DXFont::GetDesc(). ", hr);

    LOGFONT f = {
      fd.Height,
      fd.Width,
      0,
      0,
      fd.Weight,
      static_cast<BYTE>(fd.Italic),
      0,
      0,
      fd.CharSet,
      fd.OutputPrecision,
      CLIP_DEFAULT_PRECIS,
      QUALITY, //fd.Quality,
      fd.PitchAndFamily,
    };
    _tcscpy(f.lfFaceName, fd.FaceName);
    *lf = f;
  }

}