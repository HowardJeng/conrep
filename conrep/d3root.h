// Interface for shared Direct3D resources
#ifndef CONREP_D3ROOT_H
#define CONREP_D3ROOT_H

#include <d3d9.h>
#include <d3dx9.h>
#include <boost/shared_ptr.hpp>

#include "atl.h"
#include "color_table.h"
#include "windows.h"

namespace console {
  struct Dimension;

  const int SPRITE_BEGIN_FLAGS = D3DXSPRITE_ALPHABLEND |
                                 D3DXSPRITE_DONOTMODIFY_RENDERSTATE |
                                 D3DXSPRITE_DONOTSAVESTATE |
                                 D3DXSPRITE_DO_NOT_ADDREF_TEXTURE;

  typedef IDirect3D9          Direct3D;
  typedef IDirect3DTexture9   Texture;
  typedef IDirect3DDevice9    Device;
  typedef IDirect3DSurface9   Surface;
  typedef ID3DXSprite         Sprite;
  typedef IDirect3DSwapChain9 SwapChain;
  typedef ID3DXFont           Font;
  
  typedef ATL::CComPtr<Direct3D>  Direct3DPtr;
  typedef ATL::CComPtr<Texture>   TexturePtr;
  typedef ATL::CComPtr<Device>    DevicePtr;
  typedef ATL::CComPtr<Surface>   SurfacePtr;
  typedef ATL::CComPtr<Sprite>    SpritePtr;
  typedef ATL::CComPtr<SwapChain> SwapChainPtr;
  typedef ATL::CComPtr<ID3DXFont> FontPtr;

  class __declspec(novtable) IDirect3DRoot {
    public:
      virtual ~IDirect3DRoot() = 0;
      
      virtual DevicePtr    device(void) const = 0;
      virtual SpritePtr    sprite(void) const = 0;
      virtual TexturePtr   background_texture(HMONITOR monitor) const = 0;
      virtual TexturePtr   white_texture(void) const = 0;
      virtual TexturePtr   create_texture(Dimension dim) = 0;
      virtual TexturePtr   create_texture(Dimension dim, D3DCOLOR color) = 0;
      virtual SwapChainPtr get_swap_chain(HWND hwnd, Dimension client_dim) = 0;
      
      virtual void reset_background(void) = 0;
      
      virtual void begin_scene(void) = 0;
      virtual void end_scene(void)   = 0;
      
      virtual void set_render_target(TexturePtr texture) = 0;
      virtual void set_render_target(SurfacePtr surface) = 0;
      virtual void clear(D3DCOLOR color) = 0;
      
      virtual bool is_device_lost(void) = 0;
      virtual void set_device_lost(void) = 0;
      
      virtual HRESULT try_recover(void) = 0;

      virtual ColorTable & get_color_table(void) = 0;
  };
  typedef boost::shared_ptr<IDirect3DRoot> RootPtr;
  RootPtr get_direct3d_root(HWND hwnd);

  class SceneLock {
    public:
      SceneLock(IDirect3DRoot & root) : root_(root) {
        root_.begin_scene();
      }
      ~SceneLock() {
        root_.end_scene();
      }
    private:
      IDirect3DRoot & root_;
      SceneLock(const SceneLock &);
      SceneLock & operator=(const SceneLock &);
  };
 
}

#endif