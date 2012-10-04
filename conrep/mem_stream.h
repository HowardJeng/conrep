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