#pragma once

#include <unordered_set>

#include "physxlib.h"

#include "utils/debug.h"
#include "world/object.h"

namespace pbx
{

using namespace physx;

namespace simsettings
{
static constexpr const char *PVD_HOST = "localhost";
static constexpr int PVD_PORT = 5425;
static constexpr int PVD_TIMEOUT_MS = 10;
static constexpr uint32_t CPU_CORES = 2;
static const PxVec3 GRAVITY = { 0,-9.8f,0 };
}

struct PhysixDebugWindow : LogsWindow, PxErrorCallback
{
  PhysixDebugWindow() : LogsWindow("PhysX") {}
  virtual void reportError(PxErrorCode::Enum code, const char *message, const char *file, int line) override;
};

class SimulationEventHandler : public PxSimulationEventCallback, public PxContactModifyCallback
{
  virtual void onConstraintBreak(PxConstraintInfo *constraints, PxU32 count) override {}
  virtual void onWake(PxActor **actors, PxU32 count) override {}
  virtual void onSleep(PxActor **actors, PxU32 count) override {}
  virtual void onTrigger(PxTriggerPair *pairs, PxU32 count) override {}
  virtual void onAdvance(const PxRigidBody *const *bodyBuffer, const PxTransform *poseBuffer, const PxU32 count) override {}
  virtual void onContactModify(PxContactModifyPair *const pairs, PxU32 count) override {}

  virtual void onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 nbPairs) override;

public:
  void broadcastCollisions();
  void registerCollision(pbl::WorldObject *b0, pbl::WorldObject *b1);

private:
  std::unordered_set<std::pair<pbl::WorldObject *, pbl::WorldObject *>, hash_pair> m_collisions;
};

PxFilterData objectPtrToFilterData(void *objectPtr);
void *filterDataToObjectPtr(const PxFilterData &filterData);

PxFilterFlags contactReportFilterShader(
  PxFilterObjectAttributes attributes0, PxFilterData filterData0,
  PxFilterObjectAttributes attributes1, PxFilterData filterData1,
  PxPairFlags &pairFlags, const void *constantBlock, PxU32 constantBlockSize);

class Physics;

struct PhysXGlobals
{
  PhysixDebugWindow       simulationLogs;
  PxDefaultAllocator      allocator;
  PxFoundation           *foundations = nullptr;
  PxPvd                  *pvd = nullptr;
  PxPhysics              *sdk = nullptr;
  PxDefaultCpuDispatcher *cpuDispatcher = nullptr;
  SimulationEventHandler  simulationEventsHandler;

  Physics                *activePhysics;
} extern *g_physxGlobals;

}