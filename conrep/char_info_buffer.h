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