#ifndef CONREP_TEXT_RENDERER_H
#define CONREP_TEXT_RENDERER_H

#include <vector>

#include "char_info_buffer.h"
#include "context_menu.h"
#include "d3root.h"
#include "dimension.h"
#include "shell_process.h"
#include "windows.h"

namespace console {
  class TextRenderer {
    public:
      TextRenderer(RootPtr & root, const Settings & settings);

      bool choose_font(DevicePtr & device, HWND hWnd);
      Dimension console_dim_from_window_size(Dimension window_dim, INT scrollbar_width, DWORD style);
      void create_texture(RootPtr & root, Dimension client_dim);
      void dispose(void);
      void draw_cursor(SpritePtr & sprite);
      Dimension get_client_size(void);
      bool poll_console_size(ProcessLock & pl);
      void recreate_font(DevicePtr & device);
      void render(SpritePtr & sprite, D3DCOLOR color);
      void resize_buffers(Dimension new_console_dim);
      void resize_process(ProcessLock & pl);
      void set_menu_options(MenuPtr & menu);
      void size_to_window_dim(Dimension window_dim, INT scrollbar_width, DWORD style);
      void size_to_work_area(RECT work_area, INT scrollbar_width, DWORD style);
      void toggle_extended_chars(void);
      void update_text_buffer(ProcessLock & pl, RootPtr & root, SpritePtr & sprite, bool active);
    private:
      TexturePtr white_texture_;
      TexturePtr text_texture_;
      FontPtr font_;
      LOGFONT lf_;

      Dimension char_dim_;
      Dimension console_dim_;
      COORD cursor_pos_;

      int gutter_size_;   // manually enforced inside border width
      bool extended_chars_;
      bool intensify_;
        
      std::vector<TCHAR> plane_buffer_; // work buffers for painting console
      CharInfoBuffer char_info_buffer_; //   window. Member variables to avoid
      // the cost of creation/deletion in every text repaint call.

      unsigned char active_pre_alpha_;
      unsigned char inactive_pre_alpha_;

      TextRenderer(const TextRenderer &);
      TextRenderer & operator=(const TextRenderer &);

      void draw_block(SpritePtr & sprite, int x, int y, D3DCOLOR color);
    };

}

#endif