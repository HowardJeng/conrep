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