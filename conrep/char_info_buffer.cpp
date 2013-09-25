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

#include "char_info_buffer.h"

#include "dimension.h"

namespace console {
  CharInfoBuffer::CharInfoBuffer() : size_(0), cache_valid_(false) {}

  void CharInfoBuffer::invalidate(void) {
    cache_valid_ = false;
  }

  void CharInfoBuffer::resize(Dimension new_size) {
    size_ = new_size.height * new_size.width;
    invalidate();
    ASSERT(buffer_.size() == cache_.size());
    if (size_ > buffer_.size()) {
      buffer_.reserve(size_);
      buffer_.resize(buffer_.capacity());
      cache_.resize(buffer_.size());
    }
  }

  bool CharInfoBuffer::match(void) const {
    ASSERT(buffer_.size() == cache_.size());
    if (!cache_valid_) return false;
    for (size_t i = 0; i < size_; ++i) {
      const CHAR_INFO & b = buffer_[i];
      const CHAR_INFO & c = cache_[i];
      if (b.Attributes != c.Attributes) return false;
      #ifdef UNICODE
        if (b.Char.UnicodeChar != c.Char.UnicodeChar) return false;
      #else
        if (b.Char.AsciiChar != c.Char.AsciiChar) return false;
      #endif
    }
    return true;
  }

  const CHAR_INFO & CharInfoBuffer::operator[](size_t index) const { return buffer_[index]; }
        CHAR_INFO & CharInfoBuffer::operator[](size_t index)       { return buffer_[index]; }

  void CharInfoBuffer::swap(void) { 
    buffer_.swap(cache_);
    cache_valid_ = true;
  }
}
