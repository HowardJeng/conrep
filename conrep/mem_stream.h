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

// In memory IStream

#ifndef CONREP_MEM_STREAM_H
#define CONREP_MEM_STREAM_H

#include "atl.h"

namespace console {
  class MemStream {
    public:
      MemStream();
      
      CComPtr<IStream> stream() { return is_; }
    private:
      CComPtr<IStream> is_;
      
      friend class MemStreamLock;
  };
  
  class MemStreamLock {
    public:
      MemStreamLock(MemStream & ms);
      ~MemStreamLock();
      
      SIZE_T size() const;
      void * addr() const;
    private:
      MemStream & ms_;
      HGLOBAL hg_;
      void * ptr_;
      
      MemStreamLock(const MemStreamLock &);
      MemStreamLock & operator=(const MemStreamLock &);
  };
}

#endif