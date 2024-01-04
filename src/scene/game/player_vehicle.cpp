#include "player_vehicle.h"

#include "player.h"
#include "display/graphical_resource.h"
#include "physics/physxlib.h"
#include "utils/debug.h"

PlayerVehicle::PlayerVehicle(pbl::GraphicalResourceRegistry &resources)
  : WorldProp(resources.loadMesh(L"res/models/player_vehicle.obj"))
{
  setLayer(pbl::Layer::PLAYER);

  m_frame = PxCreateDynamic(
    pbx::Physics::getSdk(),
    physx::PxTransform(150, 20, 150),
    physx::PxBoxGeometry(FRAME_HALF_X, FRAME_HALF_Y, FRAME_HALF_Z),
    *pbx::Physics::getSdk().createMaterial(0.5f, 0.6f, 0.3f),
    1);
  m_frame->setMass(MASS);
  m_frame->setMaxDepenetrationVelocity(PX_MAX_F32);
  m_frame->setSolverIterationCounts(255, 255);
  m_frame->setMaxLinearVelocity(Player::MAX_SPEED);
  m_frame->setMaxAngularVelocity(10.0f);
  m_frame->setLinearDamping(0.01f);
  m_frame->setAngularDamping(0.05f);
  pbx::PhysicsBody physics{ this };
  physics.addActor(m_frame);
  setPhysics(std::move(physics));
}

float PlayerVehicle::getLinearSpeed() const
{
  return m_frame->getLinearVelocity().magnitude();
}

float PlayerVehicle::getAngularSpeed() const
{
  return m_frame->getAngularVelocity().magnitude();
}
