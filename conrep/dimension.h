// Because the basic window types don't have constructors.
#ifndef CONREP_DIMENSION_H
#define CONREP_DIMENSION_H

namespace console {
  struct Dimension {
    Dimension() {}
    Dimension(int w, int h) : width(w), height(h) {}

    int width;
    int height;
  };
}

#endif