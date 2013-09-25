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

// Dimension funtions

#ifndef CONREP_DIMENSION_OPS_H
#define CONREP_DIMENSION_OPS_H

#include <algorithm>
#include "dimension.h"

namespace console {
  inline Dimension operator/(const Dimension & lhs, const Dimension & rhs) {
    return Dimension(lhs.width / rhs.width, lhs.height / rhs.height);
  }
  
  inline Dimension min(const Dimension & lhs, const Dimension & rhs) {
    return Dimension(std::min(lhs.width, rhs.width),
                     std::min(lhs.height, rhs.height));
  }
  
  inline bool operator==(const Dimension & lhs, const Dimension & rhs) {
    return (lhs.width == rhs.width) && (lhs.height == rhs.height);
  }

  inline bool operator!=(const Dimension & lhs, const Dimension & rhs) {
    return (lhs.width != rhs.width) || (lhs.height != rhs.height);
  }

}

#endif