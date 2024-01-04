#include "game_logic.h"

#include "inputs/user_inputs.h"
#include "scene/scene_manager.h"
#include "scene/game/end_menu.h"
#include "world/trigger_box.h"

using namespace pbl;

// time between when the player crosses the last checkpoint and looses control
// and when the end menu shows up
static constexpr float GAME_END_DURATION = 2.f;

GameLogic::GameLogic(UIManager *uiManager)
{
  if (uiManager != nullptr) m_gameUI.emplace(uiManager);
}

void GameLogic::logicUpdate(float delta)
{
  delta = std::min(delta, 1.f/60.f);
  m_time += delta;
  if(m_gameUI)
    m_gameUI->update(m_time, m_player->getCurrentVelocity(), m_player->getRemainingFuel());

  // transition to the end screen
  if(m_gameState == GameState::ENDED && m_timeSinceGameEnd > GAME_END_DURATION) {
    SceneManager::getInstance().pushLayeredScene(SceneManager::make_supplier<GameEndMenuScene>());
    m_gameState = GameState::POSTEND;
    m_gameUI.reset();
  }

  if(m_gameState == GameState::ENDED || m_gameState == GameState::POSTEND) {
    m_timeSinceGameEnd += delta;
    makePlayerFollowPostEndTrack(delta);
  } else {
    // fast respawn
    if (UserInputs::consumeKeyPress(keys::SC_R)) {
      setPlayerToCheckpoint();
    }
  }
}

void GameLogic::renderUI()
{
  if(m_gameUI)
    m_gameUI->render();
}

void GameLogic::setToEndGame()
{
  m_gameState = GameState::POSTEND;
  m_player->setFixedCameraActive(true);
  m_player->setCanControlVehicle(false);
}

pbx::PhysicsEventHandler::CollisionResponse GameLogic::shouldObjectsCollide(const WorldObject &b1, const WorldObject &b2)
{
  if (b1.getLayer() == Layer::TRIGGER || b2.getLayer() == Layer::TRIGGER)
		return CollisionResponse::REPORT_ONLY;
  return CollisionResponse::COLLIDE;
}

void GameLogic::onObjectCollision(WorldObject *b1, WorldObject *b2)
{
  if (b1->getLayer() != Layer::PLAYER)
    std::swap(b1, b2);
  if (b1->getLayer() == Layer::PLAYER && b2->getLayer() == Layer::TRIGGER)
    static_cast<TriggerBox *>(b2)->trigger();
}

void GameLogic::validateCheckpoint(checkpointid_t checkId, const Transform& respawnCheckpoint)
{
  if (m_gameState == GameState::ENDED || m_gameState == GameState::POSTEND)
    return;
  // a checkpoint was triggered
  if (checkId != m_currentCheckpointId + 1) return;
  // ...it was the next checkpoint in line
  m_checkpointTransform = respawnCheckpoint;
  m_player->setCheckpointTransform(respawnCheckpoint);
  if(m_gameUI)
    m_gameUI->onNewCheckpointCrossed(m_time, m_currentCheckpointId == m_lastCheckpointId);
  if (++m_currentCheckpointId < m_lastCheckpointId) return;
  // ...it was the final checkpoint
  m_currentCheckpointId = -1;
  if (m_turnsRequired != m_turnNb) return;
  // ...of the last turn
  m_currentCheckpointId = m_lastCheckpointId+1; // prevent any other checkpoint to trigger
  m_player->setFixedCameraActive(true);
  m_player->setCanControlVehicle(false);
  m_gameState = GameState::ENDED;
  m_timeSinceGameEnd = 0;
}

void GameLogic::makePlayerFollowPostEndTrack(float deltaTime)
{
  Transform currentTransform = m_player->getVehicle().getTransform();
  vec3 currentTarget = currentTransform.transform(Camera::FORWARD * 10.f);
  vec3 targetPosition = *std::ranges::min_element(m_postEndTrack.points, std::less{}, [&](vec3 p) { return XMVectorGetX(XMVector3Length(p - currentTarget)); });
  quat targetDirection = Camera::quaternionLookAt(currentTransform.position, targetPosition);
  quat newDirection = XMQuaternionSlerp(targetDirection, currentTransform.rotation, .9f);
  vec3 newPosition = currentTransform.position + XMVector3Rotate(Camera::FORWARD, newDirection) * deltaTime * 80.f;
  m_player->resetPosition(Transform{ newPosition, {1,1,1}, newDirection });
}

