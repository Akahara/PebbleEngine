#include "physics.h"

#include <stdexcept>

#include "structures.h"
#include "utils/debug.h"

using namespace physx;

namespace pbx
{

Physics::Physics()
{
  PxSceneDesc sceneDesc(g_physxGlobals->sdk->getTolerancesScale());
  sceneDesc.gravity = simsettings::GRAVITY;
  sceneDesc.cpuDispatcher = g_physxGlobals->cpuDispatcher;
  sceneDesc.filterShader = contactReportFilterShader;
  sceneDesc.simulationEventCallback = &g_physxGlobals->simulationEventsHandler;
  sceneDesc.contactModifyCallback = &g_physxGlobals->simulationEventsHandler;
  m_scene = g_physxGlobals->sdk->createScene(sceneDesc);

  PxPvdSceneClient *pvdClient = m_scene->getScenePvdClient();
  PBL_DEBUG(if (!pvdClient) throw std::runtime_error("PhysX install does not support visual debugger");)
  if (pvdClient) {
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
    pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
  }
}

Physics::~Physics()
{
  m_scene->release();
}

void Physics::addBody(PhysicsBody *body)
{
  PBL_ASSERT(body, "body is null");
  if (body->getActors().empty()) throw std::runtime_error("Tried to add a physics body without actors to a simulation");
  m_scene->addActors(reinterpret_cast<PxActor*const*>(body->getActors().data()), static_cast<PxU32>(body->getActors().size()));
  m_physicsBodies.push_back(body);
}

void Physics::clearScene()
{
  auto allActorsFlag = PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC;
  auto actorsCount = m_scene->getNbActors(allActorsFlag);
  auto actors = std::make_unique<PxActor*[]>(actorsCount);
  m_scene->getActors(allActorsFlag, actors.get(), actorsCount);
  m_scene->removeActors(actors.get(), actorsCount);
  m_physicsBodies.clear();
}

void Physics::step(double delta)
{
  g_physxGlobals->activePhysics = this;
  m_scene->simulate(static_cast<PxReal>(std::min(delta, 1/30.)));
  m_scene->fetchResults(true);

  // propagate transform updates
  for (PhysicsBody *body : m_physicsBodies) {
    PxTransform transform = body->getActors()[0]->getGlobalPose();
    Transform &worldTransform = body->getWorldCounterpart()->getTransform();
    worldTransform.position = physics2scenePosition(transform.p);
    worldTransform.rotation = physics2sceneRotation(transform.q);
  }

  // send collisions events (1 max per object pair)
  g_physxGlobals->simulationEventsHandler.broadcastCollisions();
}

void Physics::loadGlobalResources()
{
  g_physxGlobals = new PhysXGlobals;
  g_physxGlobals->foundations = PxCreateFoundation(PX_PHYSICS_VERSION, g_physxGlobals->allocator, g_physxGlobals->simulationLogs);
#ifdef PBL_ISDEBUG
  g_physxGlobals->pvd = PxCreatePvd(*g_physxGlobals->foundations);
  g_physxGlobals->pvd->connect(*PxDefaultPvdSocketTransportCreate(simsettings::PVD_HOST, simsettings::PVD_PORT, simsettings::PVD_TIMEOUT_MS), PxPvdInstrumentationFlag::eALL);
#endif
  g_physxGlobals->sdk = PxCreatePhysics(PX_PHYSICS_VERSION, *g_physxGlobals->foundations, PxTolerancesScale{}, true, g_physxGlobals->pvd);
  g_physxGlobals->cpuDispatcher = PxDefaultCpuDispatcherCreate(simsettings::CPU_CORES);
}

void Physics::unloadGlobalResources()
{
  g_physxGlobals->cpuDispatcher->release();
  g_physxGlobals->sdk->release();
#ifdef PBL_ISDEBUG
  PxPvdTransport* transport = g_physxGlobals->pvd->getTransport();
  g_physxGlobals->pvd->release();
  transport->release();
#endif
  g_physxGlobals->foundations->release();
  delete g_physxGlobals;
  g_physxGlobals = nullptr;
}

physx::PxPhysics &Physics::getSdk()
{
  return *g_physxGlobals->sdk;
}

vec3 physics2scenePosition(const physx::PxVec3 &position)
{
  return { position.z, position.y, position.x, 1 };
}

physx::PxVec3 scene2physicsPosition(const vec3 &position)
{
  return { XMVectorGetZ(position), XMVectorGetY(position), XMVectorGetX(position) };
}

physx::PxVec3 scene2physicsPosition(const rvec3 &position)
{
  return { position.z, position.y, position.x };
}

physx::PxVec3 scene2physicsScale(const vec3 &scale)
{
  return { XMVectorGetZ(scale), XMVectorGetY(scale), XMVectorGetX(scale) };
}

physx::PxQuat scene2physicsRotation(const quat &rotation)
{
  return { XMVectorGetZ(rotation), XMVectorGetY(rotation), XMVectorGetX(rotation), -XMVectorGetW(rotation) };
}

physx::PxTransform scene2physicsTransform(const Transform &transform)
{
  PBL_ASSERT(transform.hasUnitScale(), "Cannot make a physics transform out of a scene transform with a scale factor");
  return PxTransform(scene2physicsPosition(transform.position), scene2physicsRotation(transform.rotation));
}

quat physics2sceneRotation(const physx::PxQuat &rotation)
{
  return { rotation.z, rotation.y, rotation.x, -rotation.w };
}

PhysicsBody::PhysicsBody(pbl::WorldObject *worldCounterpart, BodyType type)
  : m_worldObject(worldCounterpart)
  , m_filterData(std::make_unique<PxFilterData>(objectPtrToFilterData(worldCounterpart))) // store the world object pointer into the filter data
  , m_type(type)
{
  PBL_ASSERT(type != PhysicsBody::EMPTY, "invalid physics body type");
}

PhysicsBody::PhysicsBody()
  : m_type(EMPTY), m_worldObject(nullptr)
{
}

PhysicsBody::~PhysicsBody() = default;

PhysicsBody::PhysicsBody(PhysicsBody &&moved) noexcept
  : m_filterData(std::move(moved.m_filterData))
  , m_actors(std::move(moved.m_actors))
  , m_worldObject(std::exchange(moved.m_worldObject, nullptr))
  , m_type(std::exchange(moved.m_type, EMPTY))
{
}

PhysicsBody &PhysicsBody::operator=(PhysicsBody &&moved) noexcept
{
  m_filterData = std::move(moved.m_filterData);
  m_worldObject = std::exchange(moved.m_worldObject, nullptr);
  m_actors = std::move(moved.m_actors);
  m_type = std::exchange(moved.m_type, EMPTY);
  return *this;
}

void PhysicsBody::addActor(physx::PxRigidActor *actor)
{
  PBL_ASSERT(actor != nullptr, "null actor");
  PBL_ASSERT(m_type != EMPTY, "this object is invalid, it was not created by a WorldObject");

  // attach filter data and user data to link the actor to its WorldObject counterpart
  auto shapes = std::make_unique<PxShape*[]>(actor->getNbShapes());
  actor->getShapes(shapes.get(), actor->getNbShapes());
  for (size_t i = 0; i < actor->getNbShapes(); i++) {
    shapes.get()[i]->setQueryFilterData(*m_filterData);
    shapes.get()[i]->setSimulationFilterData(*m_filterData);
    if(m_type == BodyType::TRIGGER)
      shapes.get()[i]->setFlags(PxShapeFlag::eTRIGGER_SHAPE);
  }
  actor->userData = m_worldObject;

  m_actors.push_back(actor);
}

}