// Timer constants
#ifndef CONREP_TIMER_H
#define CONREP_TIMER_H

namespace console {
  enum {
    TIMER_REPAINT       = 0x101,
    TIMER_POLL_REGISTRY = 0x102,
    REPAINT_TIME        = 250,
    POLL_TIME           = 250
  };
}

#endif