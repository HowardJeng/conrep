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

#include "color_table.h"

#include "assert.h"
#include "exception.h"

namespace console {
  ColorTable::ColorTable() {
    LONG ret_val = console_.Open(HKEY_CURRENT_USER, _T("Console"), KEY_READ);
    if (ret_val != ERROR_SUCCESS) {
      WIN_EXCEPT("Failure opening Console desktop key. ");
    }

    assign_table();

    HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (event == NULL) WIN_EXCEPT("Failed call to CreateEvent(). ");
    event_.Attach(event);

    ret_val = console_.NotifyChangeKeyValue(FALSE, REG_NOTIFY_CHANGE_LAST_SET, event, TRUE);
    if (ret_val != ERROR_SUCCESS) WIN_EXCEPT2("Failed call to CRegKey::NotifyChangeKeyValue(). ", ret_val);
  }

  D3DCOLOR ColorTable::operator[](size_t index) const {
    ASSERT(index < CONSOLE_COLORS);
    return colors_[index];
  }

  void ColorTable::assign_table(void) {
    for (int i = 0; i < CONSOLE_COLORS; ++i) {
      const int BUFFER_SIZE = 16384; // maximum length of registry value name + 1
      TCHAR value_name[BUFFER_SIZE];
      _sntprintf(value_name, BUFFER_SIZE, _T("ColorTable%02d"), i);
      DWORD value;
      if (console_.QueryDWORDValue(value_name, value) != ERROR_SUCCESS) {
        WIN_EXCEPT("Failure reading console color value. ");
      }
      DWORD r = value & 0xff;
      DWORD g = (value >> 8) & 0xff;
      DWORD b = (value >> 16) & 0xff;
      colors_[i] = D3DCOLOR_XRGB(r, g, b);
    }
  }

  void ColorTable::poll_registry_change(void) {
    DWORD ret_val = WaitForSingleObject(event_, 0);
    if (ret_val == WAIT_FAILED ) {
      WIN_EXCEPT("Failed call to WaitForSingleObject(). ");
    } else if (ret_val == WAIT_OBJECT_0) {
      if (!ResetEvent(event_)) WIN_EXCEPT("Failed call to ResetEvent(). ");
      assign_table();
    }
  }

}