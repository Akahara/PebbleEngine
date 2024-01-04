#include "trigger_box.h"

#include "physics/physxlib.h"

namespace pbl {

TriggerBox::TriggerBox(const Transform& transform)
{
  m_layer = Layer::TRIGGER;
  m_transform = transform;
}

TriggerBox::TriggerBox(const std::function<void()>& callback, const Transform& transform)
  : TriggerBox(transform)
{
  setCallback(callback);
}

TriggerBox::TriggerBox(const std::function<void(TriggerBox& self)>& callback, const Transform& transform)
  : TriggerBox(transform)
{
  setCallback(callback);
}

void TriggerBox::setCallback(const std::function<void()>& callback)
{ m_onTriggerCallback = [callback](auto &self) { callback(); }; }

void TriggerBox::setCallback(const std::function<void(TriggerBox& self)>& callback)
{ m_onTriggerCallback = callback; }

pbx::PhysicsBody *TriggerBox::buildPhysicsObject()
{
  using namespace physx;
  using namespace pbx;

  m_physicsBody = PhysicsBody(this, PhysicsBody::TRIGGER);
  m_physicsBody.addActor(PxCreateStatic(
    Physics::getSdk(),
    PxTransform(scene2physicsPosition(m_transform.position), scene2physicsRotation(m_transform.rotation)),
    PxBoxGeometry(scene2physicsScale(m_transform.scale*.5f)),
    *Physics::getSdk().createMaterial(0.5f, 0.6f, 0.0f)));
  return &m_physicsBody;
}

}
