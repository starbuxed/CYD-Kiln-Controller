#pragma once

#include "controller_link.h"

class KilnWebApp {
 public:
  void begin(ControllerLink& link);
  void update();
};
