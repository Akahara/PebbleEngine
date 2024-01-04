#pragma once

#include "game_ui.h"
#include "player.h"
#include "utils/bezier_curve.h"

class GameLogic : public pbx::PhysicsEventHandler
{
public:
	using checkpointid_t = size_t;

	enum class GameState {
	  PLAYING, ENDED, POSTEND,
	};

private:
	Player *m_player = nullptr;

	float m_time{};
	float m_timeSinceGameEnd{};
	GameState m_gameState = GameState::PLAYING;

	checkpointid_t m_currentCheckpointId = -1;
	checkpointid_t m_lastCheckpointId = 0;
	Transform m_checkpointTransform;
	unsigned int m_turnNb = 1;
	unsigned int m_turnsRequired = 1;
	DiscreteCurve m_postEndTrack;

	// empty when no uiManager is provided to GameLogic of after the game's end
	std::optional<GameUI> m_gameUI;

public:
	explicit GameLogic(pbl::UIManager *uiManager);

	void logicUpdate(float delta);

	void renderUI();
	// immediately transition to the end-game, used in the main menu to have a self-running background game
	void setToEndGame();

protected:
	CollisionResponse shouldObjectsCollide(const pbl::WorldObject &b1, const pbl::WorldObject &b2) override;
	// collisions likely need to be debounced, objects that overlaps for multiple frames will send many consecutive events
	void onObjectCollision(pbl::WorldObject *b1, pbl::WorldObject *b2) override;

public:
	// used by checkpointBoxes
	void validateCheckpoint(checkpointid_t checkId, const Transform& respawnCheckpoint);

	// must be used at the initialization of the scene
	void setLastCheckpointId(checkpointid_t lastId) { m_lastCheckpointId = lastId; }
	void setPlayer(Player *player) { m_player = player; }
	void setNumberOfTurns(unsigned int nbTurns) { m_turnsRequired = nbTurns; }

	// resets the respawn location without triggering a checkpoint
	void setCheckpointPosition(const Transform &respawnCheckpoint) { m_checkpointTransform = respawnCheckpoint; m_player->setCheckpointTransform(respawnCheckpoint); }
	// used by killboxes
	void setPlayerToCheckpoint() { m_player->resetPosition(m_checkpointTransform); }

	// set the track the player vehicle will automatically follow once they cross the finish line
	void setPostEndTrack(DiscreteCurve curve) { m_postEndTrack = std::move(curve); }

	const Transform &getCurrentRespawnPosition() const { return m_checkpointTransform; }
	Player *getPlayer() const { return m_player; }

private:
	void makePlayerFollowPostEndTrack(float deltaTime);
};
