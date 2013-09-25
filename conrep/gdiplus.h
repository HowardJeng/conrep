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