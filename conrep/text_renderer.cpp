#include "text_renderer.h"

#include <vector>

#include "assert.h"
#include "char_info_buffer.h"
#include "console_util.h"
#include "context_menu.h"
#include "d3root.h"
#include "dimension_ops.h"
#include "exception.h"
#include "font_util.h"
#include "settings.h"
#include "shell_process.h"
#include "windows.h"

namespace console {
  const int CONSOLE_COLORS = 16; // number of different colors that a console window
                                  //   can display

  // colors used for console text
  const D3DCOLOR colors[CONSOLE_COLORS] = {
    D3DCOLOR_XRGB(0x00, 0x00, 0x00),
    D3DCOLOR_XRGB(0x00, 0x00, 0x80),
    D3DCOLOR_XRGB(0x00, 0x80, 0x00),
    D3DCOLOR_XRGB(0x00, 0x80, 0x80),
    D3DCOLOR_XRGB(0x80, 0x00, 0x00),
    D3DCOLOR_XRGB(0x80, 0x00, 0x80),
    D3DCOLOR_XRGB(0x80, 0x80, 0x00),
    D3DCOLOR_XRGB(0xC0, 0xC0, 0xC0),
    D3DCOLOR_XRGB(0x80, 0x80, 0x80), 
    D3DCOLOR_XRGB(0x00, 0x00, 0xFF),
    D3DCOLOR_XRGB(0x00, 0xFF, 0x00),
    D3DCOLOR_XRGB(0x00, 0xFF, 0xFF),
    D3DCOLOR_XRGB(0xFF, 0x00, 0x00),
    D3DCOLOR_XRGB(0xFF, 0x00, 0xFF),
    D3DCOLOR_XRGB(0xFF, 0xFF, 0x00),
    D3DCOLOR_XRGB(0xFF, 0xFF, 0xFF)
  };

  TextRenderer::TextRenderer(RootPtr & root, const Settings & settings)
    : white_texture_(root->white_texture()),
      font_(create_font(root->device(), settings.font_name, settings.font_size * POINT_SIZE_SCALE)),
      char_dim_(console::get_char_dim(font_)),
      console_dim_(Dimension(settings.columns, settings.rows)),
      gutter_size_(settings.gutter_size),
      extended_chars_(settings.extended_chars),
      intensify_(settings.intensify),
      active_pre_alpha_(static_cast<unsigned char>(settings.active_pre_alpha)),
      inactive_pre_alpha_(static_cast<unsigned char>(settings.inactive_pre_alpha)),
      font_size_(settings.font_size * POINT_SIZE_SCALE)
  {
    get_logfont(font_, &lf_);
    ASSERT(settings.active_pre_alpha <= std::numeric_limits<unsigned char>::max());
    ASSERT(settings.inactive_pre_alpha <= std::numeric_limits<unsigned char>::max());
  }

  void TextRenderer::adjust(const DevicePtr & device, const Settings & settings) {
    invalidate();
    if (settings.scl_font_name || settings.scl_font_size) {
      if (settings.scl_font_size) {
        font_size_ = settings.font_size * POINT_SIZE_SCALE;
      }
      if (settings.scl_font_name) {
        font_ = create_font(device, settings.font_name, font_size_);
      } else {
        font_ = create_font(device, lf_.lfFaceName, font_size_);
      }
      get_logfont(font_, &lf_);
      char_dim_ = console::get_char_dim(font_);
    }

    if (settings.scl_gutter_size) gutter_size_ = settings.gutter_size;
    if (settings.scl_extended_chars) extended_chars_ = settings.extended_chars;
    if (settings.scl_intensify) intensify_ = settings.intensify;
    if (settings.scl_active_pre_alpha) {
      ASSERT(settings.active_pre_alpha <= std::numeric_limits<unsigned char>::max());
      active_pre_alpha_ = static_cast<unsigned char>(settings.active_pre_alpha);
    }
    if (settings.scl_inactive_pre_alpha) {
      ASSERT(settings.inactive_pre_alpha <= std::numeric_limits<unsigned char>::max());
      inactive_pre_alpha_ = static_cast<unsigned char>(settings.inactive_pre_alpha);
    }
  }

  void TextRenderer::toggle_extended_chars(void) {
    extended_chars_ = !extended_chars_;
    char_info_buffer_.invalidate();
  }

  Dimension TextRenderer::console_dim_from_window_size(Dimension window_dim, INT scrollbar_width, DWORD style) {
    Dimension usable = get_max_usable_client_dim(window_dim, gutter_size_, scrollbar_width, style);
    return usable / char_dim_;
  }

  void TextRenderer::size_to_window_dim(Dimension window_dim, INT scrollbar_width, DWORD style) {
    resize_buffers(console_dim_from_window_size(window_dim, scrollbar_width, style));
  }

  void TextRenderer::size_to_work_area(RECT work_area, INT scrollbar_width, DWORD style) {
    Dimension window_dim = get_max_window_dim(work_area);
    size_to_window_dim(window_dim, scrollbar_width, style);
  }

  void TextRenderer::resize_process(ProcessLock & pl) {
    resize_buffers(pl.resize(console_dim_));
  }

  bool TextRenderer::choose_font(DevicePtr & device, HWND hWnd) {
    LOGFONT lf = lf_;
          
    CHOOSEFONT cf = {
      sizeof(CHOOSEFONT),
      hWnd,
      0,
      &lf,
      0,
      CF_INITTOLOGFONTSTRUCT | CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_NOSIMULATIONS | CF_NOVERTFONTS
                             | CF_SCREENFONTS
    };
    if (ChooseFont(&cf)) {
      font_ = create_font(device, lf);
      lf_ = lf;
      char_dim_ = console::get_char_dim(font_);
      font_size_ = cf.iPointSize;
      return true;
    }
    return false;
  }

  Dimension TextRenderer::get_client_size(void) {
    return calc_client_size(char_dim_, console_dim_, gutter_size_);
  }

  void TextRenderer::create_texture(RootPtr & root, Dimension client_dim) {
    white_texture_ = root->white_texture();
    text_texture_ = root->create_texture(client_dim, D3DCOLOR_ARGB(0x80, 0, 0, 0));
  }

  void TextRenderer::recreate_font(DevicePtr & device) {
    font_ = create_font(device, lf_);
  }

  void TextRenderer::dispose(void) {
    font_ = 0;
    white_texture_ = 0;
    text_texture_ = 0; 
  }

  void TextRenderer::set_menu_options(MenuPtr & menu) {
    menu->set_extended_chars(extended_chars_);
  }

  void TextRenderer::render(SpritePtr & sprite, D3DCOLOR color) {
    HRESULT hr = sprite->Draw(text_texture_, 0, 0, 0, color);
    if (FAILED(hr)) DX_EXCEPT("Failed call to ID3DXSprite::Draw(). ", hr);
  }

  bool TextRenderer::poll_console_size(ProcessLock & pl) {
    Dimension d = pl.get_console_size();
    if (d == console_dim_) return false;

    resize_buffers(d);
    return true;
  }

  void TextRenderer::resize_buffers(Dimension new_console_dim) {
    console_dim_ = new_console_dim;
    size_t new_size = new_console_dim.width * CONSOLE_COLORS;
    if (new_size > plane_buffer_.size()) {
      plane_buffer_.reserve(new_size);
      plane_buffer_.resize(plane_buffer_.capacity());
    }

    char_info_buffer_.resize(new_console_dim);
  }

  void TextRenderer::draw_block(SpritePtr & sprite, int x, int y, D3DCOLOR color) {
    RECT r = {
      0,
      0,
      char_dim_.width,
      char_dim_.height
    };
    D3DXVECTOR3 vec(static_cast<float>(gutter_size_ + x * char_dim_.width),
                    static_cast<float>(gutter_size_ + y * char_dim_.height),
                    0);
    HRESULT hr = sprite->Draw(white_texture_, &r, 0, &vec, color);
    if (FAILED(hr)) DX_EXCEPT("Failed call to ID3DXSprite::Draw(). ", hr);
  }

  void TextRenderer::draw_cursor(SpritePtr & sprite) {
    if ((cursor_pos_.X < console_dim_.width) &&
        (cursor_pos_.Y < console_dim_.height)) {
      draw_block(sprite, cursor_pos_.X, cursor_pos_.Y, 0xB0C0C0C0);
    }
  }

  void TextRenderer::invalidate(void) {
    char_info_buffer_.invalidate();
  }
        
  void TextRenderer::update_text_buffer(ProcessLock & pl, RootPtr & root, SpritePtr & sprite, bool active) {
    ASSERT(text_texture_ != nullptr);
    pl.get_console_info(console_dim_, char_info_buffer_, cursor_pos_);

    if (!char_info_buffer_.match()) {
      root->set_render_target(text_texture_);
            
      {
        SceneLock scene(*root);
              
        if (active) {
          root->clear(D3DCOLOR_ARGB(active_pre_alpha_, 0, 0, 0));
        } else {
          root->clear(D3DCOLOR_ARGB(inactive_pre_alpha_, 0, 0, 0));
        }

        for (int i = 0; i < console_dim_.height; ++i) {
          for (int j = 0; j < console_dim_.width; ++j) {
            int offset = i * console_dim_.width + j;
            int bg_index = (char_info_buffer_[offset].Attributes >> 4) & 0xf;
            if (bg_index) draw_block(sprite, j, i, colors[bg_index]);
          }
        }

        int plane_usage[CONSOLE_COLORS];
        int first_plane[CONSOLE_COLORS];

        ASSERT(((plane_buffer_.end() - plane_buffer_.begin()) % CONSOLE_COLORS) == 0);
        std::fill(plane_buffer_.begin(), plane_buffer_.end(), _T(' '));
        for (int i = 0; i < console_dim_.height; ++i) {
          std::fill_n(plane_usage, CONSOLE_COLORS, 0);
          std::fill_n(first_plane, CONSOLE_COLORS, -1);
          for (int j = 0; j < console_dim_.width; ++j) {
            int offset = i * console_dim_.width + j;
            #ifdef UNICODE
              TCHAR c = char_info_buffer_[offset].Char.UnicodeChar;
            #else
              TCHAR c = char_info_buffer_[offset].Char.AsciiChar;
            #endif
            WORD attr = char_info_buffer_[offset].Attributes;
            int fg_index = attr & 0xf;

            if (intensify_ && fg_index) fg_index |= 0x8;
                  
            if ( (c != 0) && 
                 (extended_chars_ || (_istprint(c) && !_istspace(c))) ) {
              plane_buffer_[fg_index * console_dim_.width + j] = c;
              plane_usage[fg_index] = j + 1;
              if (first_plane[fg_index] == -1) first_plane[fg_index] = j;
            }
          }
          for (int j = 0; j < CONSOLE_COLORS; ++j) {
            if (plane_usage[j]) {
              int f = first_plane[j];
              RECT r = {
                gutter_size_ + char_dim_.width * f,
                gutter_size_ + char_dim_.height * i,
                char_dim_.width * console_dim_.width + gutter_size_,
                char_dim_.height * console_dim_.height + gutter_size_
              };
              int line_start = j * console_dim_.width + f;
              int length = plane_usage[j] - f;
              HRESULT hr = font_->DrawText(sprite,
                                            &plane_buffer_[line_start],
                                            length,
                                            &r,
                                            DT_LEFT | DT_TOP | DT_NOCLIP | DT_SINGLELINE,
                                            colors[j]);
              if (FAILED(hr)) DX_EXCEPT("Failed call to ID3DXFont::DrawText(). ", hr);
              std::fill_n(plane_buffer_.begin() + line_start, length, _T(' '));
            }
          }
        }
      }
      char_info_buffer_.swap();
    }
  }

}