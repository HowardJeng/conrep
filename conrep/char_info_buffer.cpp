#include "char_info_buffer.h"

#include "dimension.h"

namespace console {
  CharInfoBuffer::CharInfoBuffer() : size_(0), cache_valid_(false) {}

  void CharInfoBuffer::resize(Dimension new_size) {
    size_ = new_size.height * new_size.width;
    cache_valid_ = false;
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
