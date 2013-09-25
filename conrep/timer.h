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