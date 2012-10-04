// Application specific window message identifiers
#ifndef CONREP_MESSAGE_H
#define CONREP_MESSAGE_H

namespace console {
  enum AppMessage {
    CRM_CONSOLE_CLOSE = WM_USER + 0,
    CRM_BACKGROUND_CHANGE,
    CRM_WORKAREA_CHANGE,
    CRM_LOST_DEVICE
  };
}

#endif