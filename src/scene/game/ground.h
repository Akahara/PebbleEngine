#pragma once

#include "world/object.h"

class Ground : public pbl::WorldProp
{
public:
  explicit Ground(pbl::GraphicalResourceRegistry &resources);
};
