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

// wrapper around two std::vector<CHAR_INFO> objects 

#ifndef CONREP_CHAR_INFO_BUFFER_H
#define CONREP_CHAR_INFO_BUFFER_H

#include <vector>

#include "assert.h"
#include "windows.h"

namespace console {
  struct Dimension;

  class CharInfoBuffer {
    public:
      CharInfoBuffer();

      void resize(Dimension new_size);
      void invalidate(void);
      bool match(void) const;

      const CHAR_INFO & operator[](size_t index) const;
            CHAR_INFO & operator[](size_t index);

      void swap(void);
    private:
      CharInfoBuffer(const CharInfoBuffer &);
      CharInfoBuffer & operator=(const CharInfoBuffer &);

      size_t size_;
      bool cache_valid_;
      std::vector<CHAR_INFO> buffer_;
      std::vector<CHAR_INFO> cache_;
  };
}

#endif