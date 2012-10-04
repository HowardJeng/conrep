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