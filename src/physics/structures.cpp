#include "structures.h"

#include <bitset>

#include "physics.h"

namespace pbx
{

PhysXGlobals *g_physxGlobals;

#pragma warning(disable : 4293 26452) // suppress "shifting by too much"
PxFilterData objectPtrToFilterData(void *objectPtr)
{
  PxU32 rawFilterData[4]{};
  static_assert(sizeof(PxU32) == sizeof(uint32_t));
  static_assert(sizeof(uintptr_t) <= sizeof(rawFilterData));
  uintptr_t worldObjectPtrAsUint = reinterpret_cast<uintptr_t>(objectPtr);
  if constexpr (sizeof(uintptr_t) * CHAR_BIT >= 0 ) rawFilterData[0] = (worldObjectPtrAsUint >> 0 ) & 0xffffffff;
  if constexpr (sizeof(uintptr_t) * CHAR_BIT >= 32) rawFilterData[1] = (worldObjectPtrAsUint >> 32) & 0xffffffff;
  if constexpr (sizeof(uintptr_t) * CHAR_BIT >= 64) rawFilterData[2] = (worldObjectPtrAsUint >> 64) & 0xffffffff;
  if constexpr (sizeof(uintptr_t) * CHAR_BIT >= 96) rawFilterData[3] = (worldObjectPtrAsUint >> 96) & 0xffffffff;
  PxFilterData filterData{ rawFilterData[0], rawFilterData[1], rawFilterData[2], rawFilterData[3] };
  return filterData;
}

void *filterDataToObjectPtr(const PxFilterData &filterData)
{
  PxU32 rawFilterData[4]{};
  static_assert(sizeof(PxU32) == sizeof(uint32_t));
  static_assert(sizeof(uintptr_t) <= sizeof(rawFilterData));
  rawFilterData[0] = filterData.word0;
  rawFilterData[1] = filterData.word1;
  rawFilterData[2] = filterData.word2;
  rawFilterData[3] = filterData.word3;
  uintptr_t worldObjectPtrAsUint = 0;
  if constexpr (sizeof(uintptr_t) * CHAR_BIT >= 0 ) worldObjectPtrAsUint |= (uintptr_t)rawFilterData[0] << 0 ;
  if constexpr (sizeof(uintptr_t) * CHAR_BIT >= 32) worldObjectPtrAsUint |= (uintptr_t)rawFilterData[1] << 32;
  if constexpr (sizeof(uintptr_t) * CHAR_BIT >= 64) worldObjectPtrAsUint |= (uintptr_t)rawFilterData[2] << 64;
  if constexpr (sizeof(uintptr_t) * CHAR_BIT >= 96) worldObjectPtrAsUint |= (uintptr_t)rawFilterData[3] << 96;
  return reinterpret_cast<void*>(worldObjectPtrAsUint);
}
#pragma warning(default : 4293 26452)

PxFilterFlags contactReportFilterShader(
  PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
  PxFilterObjectAttributes attributes1, PxFilterData filterData1,
  PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
  if (filterData0 != PxFilterData() && filterData1 != PxFilterData()) {
    pbl::WorldObject *b0 = static_cast<pbl::WorldObject*>(filterDataToObjectPtr(filterData0));
    pbl::WorldObject *b1 = static_cast<pbl::WorldObject*>(filterDataToObjectPtr(filterData1));

    for (PhysicsEventHandler *handler : g_physxGlobals->activePhysics->getEventHandlers()) {
      PhysicsEventHandler::CollisionResponse response = handler->shouldObjectsCollide(*b0, *b1);
      if(response == PhysicsEventHandler::CollisionResponse::IGNORE)
        return PxFilterFlag::eKILL;
      if(response == PhysicsEventHandler::CollisionResponse::REPORT_ONLY) {
        g_physxGlobals->simulationEventsHandler.registerCollision(b0, b1);
        return PxFilterFlag::eKILL;
      }
    }
  }

  pairFlags = 
    PxPairFlag::eCONTACT_DEFAULT       |
    PxPairFlag::eNOTIFY_TOUCH_FOUND    |
    PxPairFlag::eNOTIFY_CONTACT_POINTS |
    PxPairFlag::eMODIFY_CONTACTS       ;
  return PxFilterFlag::eDEFAULT;
}

void PhysixDebugWindow::reportError(PxErrorCode::Enum code, const char *message, const char *file, int line)
{
  constexpr size_t physxErrorFlagsLength = 7;
  logm("Error ", std::bitset<physxErrorFlagsLength>(code), " at ", file, ":", line, ": ", message);
}

void SimulationEventHandler::onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 nbPairs)
{
  for (PxU32 i = 0; i < nbPairs; i++) {
    const PxContactPair &pair = pairs[i];
    pbl::WorldObject *b0 = static_cast<pbl::WorldObject *>(pair.shapes[0]->getActor()->userData);
    pbl::WorldObject *b1 = static_cast<pbl::WorldObject *>(pair.shapes[1]->getActor()->userData);
    registerCollision(b0, b1);
  }
}

void SimulationEventHandler::broadcastCollisions()
{
  for (auto &[b1, b2] : m_collisions) {
    for (PhysicsEventHandler *handler : g_physxGlobals->activePhysics->getEventHandlers())
      handler->onObjectCollision(b1, b2);
  }
  m_collisions.clear();
}

void SimulationEventHandler::registerCollision(pbl::WorldObject* b0, pbl::WorldObject* b1)
{
  if (b0 < b1) std::swap(b0, b1);
  m_collisions.insert(std::make_pair(b0, b1));
}
}
