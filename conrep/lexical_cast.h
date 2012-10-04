// wrap the inclusion of boost/lexical_cast.hpp and silence warnings from its inclusion
#ifndef CONREP_LEXICAL_CAST_H
#define CONREP_LEXICAL_CAST_H

#include "tchar.h"
#pragma warning(push)
  #pragma warning(disable:4127)
  #pragma warning(disable:4511)
  #pragma warning(disable:4512)
  #pragma warning(disable:4671)
  #pragma warning(disable:4673)
  #pragma warning(disable:4701)
  #include <boost/lexical_cast.hpp>
#pragma warning(pop)

#endif