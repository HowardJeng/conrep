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