// d3root.cpp
// implementation of Direct3D root object that contains shared resources for
//   various console windows
// TODO: move various helper functions/classes into other file(s)

#include "d3root.h"

#include "assert.h"
#include "dimension.h"
#include "exception.h"
#include "gdiplus.h"
#include "reg.h"
#include "mem_stream.h"

#include <map>
#include <sstream>

using namespace ATL;
using namespace Gdiplus;

namespace console {
  CLSID GetEncoderClsid(const std::wstring & format) {
    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    Status st = GetImageEncodersSize(&num, &size);
    if (st != Ok) GDIPLUS_EXCEPT("Failed call to GetImageEncodersSize(). ", st);

    std::vector<char> buffer(size);
    ImageCodecInfo * pImageCodecInfo = reinterpret_cast<ImageCodecInfo *>(&buffer[0]);

    st = GetImageEncoders(num, size, pImageCodecInfo);
    if (st != Ok) GDIPLUS_EXCEPT("Failed call to GetImageEncoders(). ", st);

    for (UINT j = 0; j < num; ++j) {
      if (pImageCodecInfo[j].MimeType == format) {
        return pImageCodecInfo[j].Clsid;
      }
    }
    MISC_EXCEPT("PNG codec not found.");
  }

  D3DPRESENT_PARAMETERS get_present_parameters(void) {  
    D3DPRESENT_PARAMETERS present_parameters = {};
    present_parameters.BackBufferCount = 1;
    present_parameters.Windowed = TRUE;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferFormat = D3DFMT_UNKNOWN;
    present_parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    return present_parameters;
  }

  template <typename T, typename CharT>
  T extract(std::basic_istream<CharT> & input_stream) {
    T temp;
    input_stream >> temp;
    return temp;
  }

  class Direct3DRoot;
  
  class Direct3DRoot : public IDirect3DRoot {
    public:
      Direct3DRoot(HWND hwnd);
      ~Direct3DRoot();
      
      DevicePtr device(void) const {
        return device_;
      }
      
      void init_sprite(void);
      SpritePtr sprite(void) const {
        return sprite_;
      }
      
      TexturePtr background_texture(HMONITOR monitor) const {
        std::map<HMONITOR, TexturePtr>::const_iterator itr = background_textures_.find(monitor);
        if (itr == background_textures_.end()) MISC_EXCEPT("Invalid HMONITOR index");
        return itr->second;
      }
      
      TexturePtr white_texture(void) const {
        return white_texture_;
      }

      TexturePtr create_texture(Dimension dim);
      TexturePtr create_texture(Dimension dim, D3DCOLOR color);
      SwapChainPtr get_swap_chain(HWND hwnd, Dimension client_dim);
      
      void reset_background(void);

      void begin_scene(void);
      void end_scene(void);

      void set_render_target(TexturePtr texture);
      void set_render_target(SurfacePtr surface);
      
      void clear(D3DCOLOR color);

      bool is_device_lost(void);
      void set_device_lost(void);
      
      HRESULT try_recover(void);

      ColorTable & get_color_table(void);
    private:
      Direct3DPtr iface_;
      DevicePtr   device_;
      SpritePtr   sprite_;
      TexturePtr  white_texture_;
      std::map<HMONITOR, TexturePtr> background_textures_;
      bool device_lost_;

      ColorTable color_table_;
    
      Direct3DRoot(const Direct3DRoot &);
      Direct3DRoot & operator=(const Direct3DRoot &);
      
      void set_background_textures(void);
      void check_capability(void);

      static BOOL CALLBACK set_background_enum_proc(HMONITOR hMonitor,
                                                    HDC hdcMonitor,
                                                    LPRECT lprcMonitor,
                                                    LPARAM dwData); 
  };
  
  void Direct3DRoot::check_capability(void) {
    D3DCAPS9 caps;
    HRESULT hr = device_->GetDeviceCaps(&caps);
    if (FAILED(hr)) DX_EXCEPT("Failed call to IDirect3DDevice9::GetDeviceCaps(). ", hr);
    
    int conditional   = caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL;
    int unconditional = caps.TextureCaps & D3DPTEXTURECAPS_POW2;
    if (!conditional && unconditional) {
      MISC_EXCEPT("Direct3D device doesn't seem to support non-power of two textures.");
    }
    
    SurfacePtr render_target;
    hr = device_->GetRenderTarget(0, &render_target);
    if (FAILED(hr)) DX_EXCEPT("Failed call to IDirect3DDevice9::GetRenderTarget(). ", hr);
    
    D3DSURFACE_DESC surface_desc = {};
    hr = render_target->GetDesc(&surface_desc);
    if (FAILED(hr)) DX_EXCEPT("Failed call to IDirect3DSurface9::GetDesc(). ", hr);
    
    D3DFORMAT target_format = surface_desc.Format;
    hr = iface_->CheckDeviceFormatConversion(0, 
                                             D3DDEVTYPE_HAL, 
                                             D3DFMT_A8R8G8B8,
                                             target_format);
    if (FAILED(hr)) {
      if (hr == D3DERR_NOTAVAILABLE) {
        MISC_EXCEPT("Display adapter doesn't appear to support required texture formats.");
      } else {
        DX_EXCEPT("Failed call to IDirect3D9::CheckDeviceFormatConversion(). ", hr);
      }
    }
  }

  Direct3DRoot::Direct3DRoot(HWND hwnd) : device_lost_(false) {
    // Swap these two lines to force a memory leak.
    //iface_ = Direct3DCreate9(D3D_SDK_VERSION);
    iface_.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    if (!iface_) MISC_EXCEPT("Unable to initialize Direct3D 9.");
  
    D3DPRESENT_PARAMETERS present_parameters = get_present_parameters();
    
    HRESULT hr = iface_->CreateDevice(D3DADAPTER_DEFAULT,
                                      D3DDEVTYPE_HAL,
                                      hwnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &present_parameters,
                                      &device_);
    if (FAILED(hr)) DX_EXCEPT("Unable to create device. ", hr);

    check_capability();
    init_sprite();
    white_texture_ = create_texture(Dimension(64, 64), D3DCOLOR_XRGB(255, 255, 255));
    set_background_textures();
  }

  void Direct3DRoot::init_sprite(void) {  
    HRESULT hr = D3DXCreateSprite(device_, &sprite_);
    if (FAILED(hr)) DX_EXCEPT("Unable to create ID3DXSprite. ", hr);

    // Prep the sprite for alpha blending; masking D3DXSPRITE_DONOTMODIFY_RENDERSTATE
    //   will modifiy the render state so that subsequent Begin() calls can specify
    //   D3DXSPRITE_DONOTMODIFY_RENDERSTATE safely later.
    hr = sprite_->Begin(SPRITE_BEGIN_FLAGS & ~D3DXSPRITE_DONOTMODIFY_RENDERSTATE);
    if (FAILED(hr)) DX_EXCEPT("Failure in ID3DXSprite::Begin(). ", hr);
    hr = sprite_->End();
    if (FAILED(hr)) DX_EXCEPT("Failure in ID3DXSprite::End(). ", hr);
  }
  
  void draw_scaled(const RECT & rect, TexturePtr wallpaper_texture, SpritePtr sprite, const D3DXVECTOR3 & center, float x_scale, float y_scale) {
    D3DXVECTOR3 position(
      center.x / x_scale - rect.right / 2,
      center.y / y_scale - rect.bottom / 2,
      0
    );
                      
    D3DXMATRIX old_matrix;
    HRESULT hr = sprite->GetTransform(&old_matrix);
    if (FAILED(hr)) DX_EXCEPT("Failure in ID3DXSprite::GetTransform(). ", hr);
    D3DXMATRIX new_matrix(old_matrix);
    new_matrix._11 *= x_scale;
    new_matrix._22 *= y_scale;
    hr = sprite->SetTransform(&new_matrix);
    if (FAILED(hr)) DX_EXCEPT("Failure in ID3DXSprite::SetTransform(). ", hr);
    hr = sprite->Draw(wallpaper_texture, &rect, 0, &position, 0xffffffff);
    if (FAILED(hr)) DX_EXCEPT("Failure in ID3DXSprite::Draw(). ", hr);
    hr = sprite->SetTransform(&old_matrix);
    if (FAILED(hr)) DX_EXCEPT("Failure in ID3DXSprite::SetTransform(). ", hr);
  }

  struct SetBackgroundData {
    Direct3DRoot * root;
    WallpaperStyle style;
    D3DCOLOR background_color;
    
    TexturePtr wallpaper_texture;
    int wallpaper_width;
    int wallpaper_height;
    
    int origin_x;
    int origin_y;
  };
  
  BOOL CALLBACK Direct3DRoot::set_background_enum_proc(HMONITOR hMonitor,
                                                       HDC,
                                                       LPRECT lprcMonitor,
                                                       LPARAM dwData) {
    SetBackgroundData * sbd = reinterpret_cast<SetBackgroundData *>(dwData);
    Direct3DRoot * root = sbd->root;
    
    int mon_width  = lprcMonitor->right - lprcMonitor->left;
    int mon_height = lprcMonitor->bottom - lprcMonitor->top;
    
    TexturePtr & texture = root->background_textures_[hMonitor];
    texture = root->create_texture(Dimension(mon_width, mon_height), 
                                   sbd->background_color);

    if (!sbd->wallpaper_texture) return TRUE;

    root->set_render_target(texture);
    SceneLock scene(*root);

    RECT rect = { 0, 0, sbd->wallpaper_width, sbd->wallpaper_height };
    float x_scale = static_cast<float>(mon_width) / sbd->wallpaper_width;
    float y_scale = static_cast<float>(mon_height) / sbd->wallpaper_height;

    D3DXVECTOR3 center(static_cast<float>(mon_width / 2),
                       static_cast<float>(mon_height / 2),
                       0);

    if (sbd->style == TILE) {
      int tx_start = (lprcMonitor->left - sbd->origin_x) / sbd->wallpaper_width;
      int ty_start = (lprcMonitor->top - sbd->origin_y) / sbd->wallpaper_height;

      int tx_stop = (lprcMonitor->right - sbd->origin_x) / sbd->wallpaper_width;
      int ty_stop = (lprcMonitor->bottom - sbd->origin_y) / sbd->wallpaper_height;
      
      for (int i = tx_start; i <= tx_stop; ++i) {
        for (int j = ty_start; j <= ty_stop; ++j) {
          D3DXVECTOR3 pos(static_cast<float>(i * sbd->wallpaper_width + sbd->origin_x - lprcMonitor->left), 
                          static_cast<float>(j * sbd->wallpaper_height + sbd->origin_y - lprcMonitor->top),
                          0);
          HRESULT hr = root->sprite_->Draw(sbd->wallpaper_texture, &rect, 0, &pos, 0xffffffff);
          if (FAILED(hr)) DX_EXCEPT("Failure in ID3DXSprite::Draw(). ", hr);
        }
      }
    } else if (sbd->style == STRETCH) {
      draw_scaled(rect, sbd->wallpaper_texture, root->sprite_, center, x_scale, y_scale);
    } else if (sbd->style == CENTER) {
      draw_scaled(rect, sbd->wallpaper_texture, root->sprite_, center, 1.0f, 1.0f);
    } else if (sbd->style == ASPECT_PAD) {
      float scale = min(x_scale, y_scale);
      draw_scaled(rect, sbd->wallpaper_texture, root->sprite_, center, scale, scale);
    } else if (sbd->style == ASPECT_CROP) {
      float scale = max(x_scale, y_scale);

      OSVERSIONINFO osvi = {};
      osvi.dwOSVersionInfoSize = sizeof(osvi);
      if (!GetVersionEx(&osvi)) WIN_EXCEPT("Failed call to GetVersionEx(). ");

      if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2) {
        // Windows 8 displays Wallpaper off center in Fill mode
        center.y += sbd->wallpaper_height / 6.0f * scale;
        center.y -= mon_height / 6.0f;
        draw_scaled(rect, sbd->wallpaper_texture, root->sprite_, center, scale, scale);
      } else {
        draw_scaled(rect, sbd->wallpaper_texture, root->sprite_, center, scale, scale);
      }
    } else {
      std::stringstream sstr;
      sstr << "Invalid wallpaper style: " << sbd->style << ". ";
      MISC_EXCEPT(sstr.str().c_str());
    }

    return TRUE;
  }
  
  void Direct3DRoot::set_background_textures(void) {
    SetBackgroundData sbd = { this };
  
    // ----- wallpaper name, tiling and style -----
    WallpaperInfo wi = get_wallpaper_info();
    sbd.style = wi.style;

    // ----- background color -----
    CRegKey colors;
    if (colors.Open(HKEY_CURRENT_USER, _T("Control Panel\\Colors"), KEY_READ)
        != ERROR_SUCCESS) WIN_EXCEPT("Failure open colors dektop key. ");
    tstringstream ss(get_string_value(colors, _T("Background")));
    int r = extract<int>(ss);
    int g = extract<int>(ss);
    int b = extract<int>(ss);
    sbd.background_color = D3DCOLOR_XRGB(r, g, b);

    // ----- set wallpaper texture -----
    if (!wi.wallpaper_name.empty()) {
      Image wallpaper(WideBuffer(wi.wallpaper_name.c_str()));
      if (wallpaper.GetLastStatus() != Ok) {
        // Unable to open wallpaper
        std::stringstream sstr;
        sstr << "Unable to open Wallpaper with name: "
             << NarrowBuffer(wi.wallpaper_name.c_str());
        GDIPLUS_EXCEPT(sstr.str().c_str(), wallpaper.GetLastStatus());
      }
      sbd.wallpaper_width  = wallpaper.GetWidth();
      sbd.wallpaper_height = wallpaper.GetHeight();

      sbd.wallpaper_texture = create_texture(Dimension(sbd.wallpaper_width, sbd.wallpaper_height));
      RECT rect = { 0, 0, sbd.wallpaper_width, sbd.wallpaper_height };

      SurfacePtr wallpaper_surface;
      HRESULT hr = sbd.wallpaper_texture->GetSurfaceLevel(0, &wallpaper_surface);
      if (FAILED(hr)) DX_EXCEPT("Failure in IDirect3DTexture9::GetSurfaceLevel(). ", hr);

      // TODO: figure out how to handle extremely large wallpaper sizes.
      hr = D3DXLoadSurfaceFromFile(wallpaper_surface, 0, &rect, wi.wallpaper_name.c_str(),
                                   0, D3DX_FILTER_NONE, 0, 0);
      if (hr == D3DXERR_INVALIDDATA) {
        // probably the file format not supported by DirectX, write a copy of the
        //   image to a memory stream as a PNG and load from that.
        CLSID pngClsid = GetEncoderClsid(L"image/png");
        MemStream ms;

        wallpaper.Save(ms.stream(), &pngClsid, 0);
        if (wallpaper.GetLastStatus() != Ok)
          GDIPLUS_EXCEPT("Image::Save() unable to write to memory stream. ", wallpaper.GetLastStatus());

        MemStreamLock msl(ms);
        ASSERT(msl.size() < 0x100000000ull);
        hr = D3DXLoadSurfaceFromFileInMemory(wallpaper_surface, 0, &rect, msl.addr(), 
                                             static_cast<UINT>(msl.size()), 0, D3DX_FILTER_NONE, 0, 0);
      }
      
      if (FAILED(hr)) {
        tostringstream sstr;
        sstr << _T("Unable to load wallpaper: ")
             << wi.wallpaper_name
             << _T(".");
        if (hr == D3DERR_INVALIDCALL) {
          sstr << _T("\nPossible cause: wallpaper is too large.");
        }
        MessageBox(0, sstr.str().c_str(), _T("Unable to load wallpaper."), MB_OK);
        //wallpaper_surface = 0;
        sbd.wallpaper_texture = 0;
      }
    }
    
    // ----- virtual desktop origin -----
    sbd.origin_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    sbd.origin_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    EnumDisplayMonitors(NULL, NULL, &set_background_enum_proc, reinterpret_cast<LPARAM>(&sbd));
  }

  TexturePtr Direct3DRoot::create_texture(Dimension dim) {
    TexturePtr ret_val;
    HRESULT hr = D3DXCreateTexture(device_, 
                                   dim.width, 
                                   dim.height,
                                   1,
                                   D3DUSAGE_RENDERTARGET,
                                   D3DFMT_A8R8G8B8,
                                   D3DPOOL_DEFAULT,
                                   &ret_val);
    if (FAILED(hr)) DX_EXCEPT("Failure in D3DXCreateTexture(). ", hr);
    return ret_val;
  } 

  TexturePtr Direct3DRoot::create_texture(Dimension dim, D3DCOLOR color) {
    TexturePtr ret_val = create_texture(dim);
    set_render_target(ret_val);
    clear(color);
    return ret_val;
  } 

  SwapChainPtr Direct3DRoot::get_swap_chain(HWND hwnd, Dimension client_dim) {
    SwapChainPtr swap_chain;
    D3DPRESENT_PARAMETERS pp = get_present_parameters();
    pp.BackBufferHeight = client_dim.height;
    pp.BackBufferWidth = client_dim.width;
    pp.hDeviceWindow = hwnd;
    
    HRESULT hr = device_->CreateAdditionalSwapChain(&pp, &swap_chain);
    if (FAILED(hr))
      DX_EXCEPT("Failure in IDirect3DDevice9::CreateAdditionalSwapChain(). ", hr);
    return swap_chain;
  }
  
  void Direct3DRoot::reset_background(void) {
    background_textures_.clear();
    set_background_textures();
  }

  void Direct3DRoot::begin_scene(void) {
    HRESULT hr = device_->BeginScene();
    if (FAILED(hr)) DX_EXCEPT("Failed call to IDirect3DDevice9::BeginScene(). ", hr);
    hr = sprite_->Begin(SPRITE_BEGIN_FLAGS);
    if (FAILED(hr)) DX_EXCEPT("Failed call to ID3DXSprite::Begin(). ", hr);
  }

  void Direct3DRoot::end_scene(void) {
    // end_scene() is intended to be called inside a destructor so doesn't
    //   throw exceptions if the function calls return errors.
    sprite_->End();
    device_->EndScene();
  }

  void Direct3DRoot::set_render_target(TexturePtr texture) {
    SurfacePtr surface;
    HRESULT hr = texture->GetSurfaceLevel(0, &surface);
    if (FAILED(hr)) DX_EXCEPT("Failure in IDirect3DTexture9::GetSurfaceLevel(). ", hr);
    set_render_target(surface);
  }

  void Direct3DRoot::set_render_target(SurfacePtr surface) {
    HRESULT hr = device_->SetRenderTarget(0, surface);
    if (FAILED(hr))
      DX_EXCEPT("Failed call to IDirect3DDevice9::SetRenderTarget(). ", hr);
  }

  void Direct3DRoot::clear(D3DCOLOR color) {
    HRESULT hr = device_->Clear(0, 0, D3DCLEAR_TARGET, color, 1.0f, 0);
    if (FAILED(hr)) DX_EXCEPT("Failed call to IDirect3DDevice9::Clear(). ", hr);
  }

  bool Direct3DRoot::is_device_lost(void) {
    return device_lost_;
  }
  
  void Direct3DRoot::set_device_lost(void) {
    device_lost_ = true;
  }
  
  // before try_recover() is called, all the console windows must free their DirectX
  //   resources
  HRESULT Direct3DRoot::try_recover(void) {
    ASSERT(device_lost_);
    HRESULT hr = device_->TestCooperativeLevel();
    ASSERT(hr != D3D_OK);
    if (hr == D3DERR_DEVICENOTRESET) {
      sprite_ = 0;
      white_texture_ = 0;
      background_textures_.clear();

      D3DPRESENT_PARAMETERS present_parameters = get_present_parameters();
      
      hr = device_->Reset(&present_parameters);
      if (FAILED(hr)) return hr;

      init_sprite();
      white_texture_ = create_texture(Dimension(64, 64), D3DCOLOR_XRGB(255, 255, 255));
      set_background_textures();

      device_lost_ = false;
      return D3DERR_DEVICENOTRESET;
    } else {
      return hr;
    }
  }

  ColorTable & Direct3DRoot::get_color_table(void) {
    return color_table_;
  }

  Direct3DRoot::~Direct3DRoot() {
    background_textures_.clear();
    white_texture_.Release();
    sprite_.Release();  
     
    device_.Release();
    iface_.Release();
  }
  
  IDirect3DRoot::~IDirect3DRoot() {}

  RootPtr get_direct3d_root(HWND hwnd) {
    return RootPtr(new Direct3DRoot(hwnd));
  }
}