#pragma once

#include <algorithm>
#include <memory>
#include <vector>
#include <physx/foundation/PxVec3.h>
#include <physx/foundation/PxQuat.h>

#include "utils/math.h"
#include "world/transform.h"

namespace physx
{

class PxScene;
class PxMaterial;
class PxRigidActor;
class PxPhysics;
struct PxFilterData;

}

namespace pbl
{

class WorldObject;

}

namespace pbx
{

vec3 physics2scenePosition(const physx::PxVec3 &position);
quat physics2sceneRotation(const physx::PxQuat &rotation);
physx::PxVec3 scene2physicsPosition(const vec3 &position);
physx::PxVec3 scene2physicsPosition(const rvec3 &position);
physx::PxVec3 scene2physicsScale(const vec3 &scale);
physx::PxQuat scene2physicsRotation(const quat &rotation);
physx::PxTransform scene2physicsTransform(const Transform &transform);

class PhysicsBody
{
public:
  enum BodyType { EMPTY, SIMULATION, TRIGGER };
  enum PhysxType { STATIC, DYNAMIC };

  explicit PhysicsBody(pbl::WorldObject *worldCounterpart, BodyType type=SIMULATION);
  PhysicsBody();
  ~PhysicsBody();
  PhysicsBody(const PhysicsBody &) = delete;
  PhysicsBody &operator=(const PhysicsBody &) = delete;
  PhysicsBody(PhysicsBody &&moved) noexcept;
  PhysicsBody &operator=(PhysicsBody &&moved) noexcept;

  PhysicsBody(pbl::WorldObject *worldCounterpart, std::initializer_list<physx::PxRigidActor*> actors)
    : PhysicsBody(worldCounterpart)
  {
    std::ranges::for_each(actors, [this](auto &a) { addActor(a); });
  }

  /*
    Every PhysicsBody needs at least one actor. There is currently no interface
    to create actors without including the physx lib, try to minimise the number
    of .cpp files including physxlib.h and do not include it in any header.
    The first actor added is the counter part of the world object. Meaning its
    the transform of the world object is updated after the first actor.
  */
  void addActor(physx::PxRigidActor *actor);
  
  BodyType getType() const { return m_type; }
  const std::vector<physx::PxRigidActor *> &getActors() const { return m_actors; }
  pbl::WorldObject *getWorldCounterpart() { return m_worldObject; }

private:
  pbl::WorldObject *m_worldObject;
  std::unique_ptr<physx::PxFilterData> m_filterData;
  std::vector<physx::PxRigidActor *> m_actors;
  BodyType m_type;
};

class PhysicsEventHandler
{
public:
  virtual ~PhysicsEventHandler() = default;

  enum class CollisionResponse { COLLIDE, IGNORE, REPORT_ONLY };

  // if multiple event handlers are bound, if at least one says objects should not collide, they will not
  virtual CollisionResponse shouldObjectsCollide(const pbl::WorldObject &b1, const pbl::WorldObject &b2) { return CollisionResponse::COLLIDE; }
  // collisions likely need to be debounced, objects that overlaps for multiple frames will send many consecutive events
  virtual void onObjectCollision(pbl::WorldObject *b1, pbl::WorldObject *b2) {}
};

class Physics
{
public:
  Physics();
  ~Physics();

  Physics(const Physics&) = delete;
  Physics &operator=(const Physics&) = delete;
  Physics(Physics&&) = delete;
  Physics &operator=(Physics&&) = delete;

  void addBody(PhysicsBody *body);
  void addEventHandler(PhysicsEventHandler *handler) { getEventHandlers().push_back(handler); }
  std::vector<PhysicsEventHandler*> &getEventHandlers() { return m_eventHandlers; }

  physx::PxScene *getScene() { return m_scene; }
  void clearScene();

  void step(double delta);

  static void loadGlobalResources();
  static void unloadGlobalResources();
  static physx::PxPhysics &getSdk();

private:
  physx::PxScene                   *m_scene = nullptr;
  std::vector<PhysicsEventHandler*> m_eventHandlers;
  std::vector<PhysicsBody*>         m_physicsBodies;
};

}
