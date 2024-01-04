#pragma once

#include "world/object.h"

namespace physx
{
  class PxRigidDynamic;
}

class PlayerVehicle : public pbl::WorldProp {
public:
  static constexpr float MASS = 1.0f;
  static constexpr float FRAME_HALF_X = 2.0f;
  static constexpr float FRAME_HALF_Y = 0.5f;
  static constexpr float FRAME_HALF_Z = 1.0f;

public:
  explicit PlayerVehicle(pbl::GraphicalResourceRegistry &resources);

  physx::PxRigidDynamic *getFrame() const { return m_frame; }
  float getLinearSpeed() const;
  float getAngularSpeed() const;

private:
  physx::PxRigidDynamic *m_frame;
};
