#pragma once

#include "game_logic.h"
#include "game_vfx.h"
#include "sun.h"
#include "scene/scene.h"
#include "display/graphical_resource.h"
#include "display/skybox.h"
#include "display/ui/ui_manager.h"
#include "engine/device.h"
#include "physics/physics.h"
#include <vector>

class MainMenuScene : public pbl::Scene
{
public:
  MainMenuScene();

  void update(double delta) override;
  void render() override;

private:
  void rebuildUI();
  void loadBackgroundGame();
  void updateBackgroundGame(float delta);
  pbl::Texture renderBackgroundGame();

  void onSceneStateChange(pbl::SceneStateChange change) override;

private:
  // UI and resources
  std::shared_ptr<pbl::ScreenResizeEventHandler> m_windowResizeEventHandler;
  pbl::GraphicalResourceRegistry m_resources;
  pbl::UIManager m_ui;
  pbl::UIManager m_persistentUI;
  pbl::ScreenRegion m_backgroundGameRegion;
  pbl::ScreenRegion m_backgroundUIRegion;
  pbl::ScreenRegion m_logoRegion;
  pbl::ScreenRegion m_startRegion;
  bool m_isSubMenuActive = false;

  // background game objects
  float m_worldTime = 0;
  pbl::GenericBuffer m_worldConstantBuffer;
  pbl::FrameBufferPipeline m_graphicsPipeline;
  pbl::SpriteRenderer m_sprites;
  std::shared_ptr<Sun> m_sun;
  pbl::Skybox m_skybox;
  Player m_player;
  GameVFX m_graphicsVFXPass;
  pbx::Physics m_physics;
  GameLogic m_gameLogic;
  std::vector<std::shared_ptr<pbl::WorldObject>> m_objects;
  std::vector<std::shared_ptr<pbl::WorldObject>> m_shadowCastingObjects;
  std::vector<std::shared_ptr<pbl::ButtonElement>> m_buttons;
  int m_selectedButtonIndex{ -1 };
};
