// wrap the inclusion of gdiplus.h
#ifndef CONREP_GDIPLUS_H
#define CONREP_GDIPLUS_H

// annoyingly, gdiplus.h depends on the windows.h min() and max() macros, but I don't want
//   those defined in the because they interfere with std::min() and std::max(),
//   therefore use windows.h with NOMINMAX but put std::min() and std::max() in the root
//   namespace
#include "windows.h"
#include <algorithm>
using std::min;
using std::max;
#include <GdiPlus.h>

#endif