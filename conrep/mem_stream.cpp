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

#include "mem_stream.h"
#include "exception.h"

namespace console {
  MemStream::MemStream() {
    HRESULT hr = CreateStreamOnHGlobal(0, // stream creates the HGLOBAL,
                                       TRUE,
                                       &is_);
    if (FAILED(hr)) WIN_EXCEPT("Failed call to CreateStreamOnHGlobal. ");
  }
  
  MemStreamLock::MemStreamLock(MemStream & ms) : ms_(ms) {
    HRESULT hr = GetHGlobalFromStream(ms_.is_, &hg_);
    if (FAILED(hr)) WIN_EXCEPT("Failed call to GetHGlobalFromStream(). ");
    ptr_ = GlobalLock(hg_);
    if (!ptr_) WIN_EXCEPT("Failed call to GlobalLock(). ");
  }
  
  MemStreamLock::~MemStreamLock() {
    GlobalUnlock(hg_);
  }
  
  SIZE_T MemStreamLock::size() const {
    SIZE_T size = GlobalSize(hg_);
    if (!size) WIN_EXCEPT("Failed call to GlobalSize(). ");
    return size;
  }
  
  void * MemStreamLock::addr() const {
    return ptr_;
  }

}