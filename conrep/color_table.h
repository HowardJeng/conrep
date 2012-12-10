#ifndef CONREP_COLOR_TABLE_H
#define CONREP_COLOR_TABLE_H

#include "windows.h"
#include <atlbase.h>
#include <d3d9types.h>

namespace console {
  class ColorTable {
    public:
      static const int CONSOLE_COLORS = 16; // number of different colors that a console window can display

      ColorTable();
      D3DCOLOR operator[](size_t index) const;

      void poll_registry_change(void);
    private:
      D3DCOLOR colors_[CONSOLE_COLORS];
      CRegKey console_;
      CHandle event_;

      void assign_table(void);

      ColorTable(const ColorTable &);
      ColorTable & operator=(const ColorTable &);
  };

}

#endif