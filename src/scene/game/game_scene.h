#pragma once

#include <memory>
#include <vector>

#include "display/graphical_resource.h"
#include "display/skybox.h"
#include "display/text.h"
#include "physics/physics.h"
#include "world/object.h"
#include "scene/scene.h"
#include "game_logic.h"
#include "game_vfx.h"
#include "player.h"
#include "sun.h"
#include "display/frame_buffer.h"


class GameScene : public pbl::Scene
{
public:
  GameScene();

  void update(double delta) override;
  void render() override;

private:
  void initSceneObjects();

  void onSceneStateChange(pbl::SceneStateChange change) override;

private:
  // -- graphics
  pbl::GraphicalResourceRegistry                 m_graphicalResources;
  pbl::FrameBufferPipeline                       m_graphicsPipeline;
  GameVFX                                        m_graphicsVFXPass;
  std::shared_ptr<pbl::GenericBuffer>            m_worldConstantBuffer;
  pbl::SpriteRenderer                            m_sprites;
  pbl::TextRenderer                              m_text;
  pbl::Skybox                                    m_skybox;
  pbl::UIManager                                 m_ui;
  // -- world
  pbx::Physics                                   m_physics;
  std::vector<std::shared_ptr<pbl::WorldObject>> m_objects;
  std::shared_ptr<Sun>                           m_sun;
  std::shared_ptr<PlayerVehicle>                 m_playerVehicle;
  std::vector<std::shared_ptr<pbl::WorldObject>> m_shadowCastingObjects;
  Player                                         m_player;
  GameLogic                                      m_gameLogic;
  double                                         m_worldTime = 0;
};
