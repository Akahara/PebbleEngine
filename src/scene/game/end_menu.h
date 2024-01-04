#pragma once

#include "scene/scene.h"
#include "display/graphical_resource.h"
#include "display/ui/ui_manager.h"
#include "engine/device.h"
#include "display/ui/ui_elements.h"
#include <vector>

class GameEndMenuScene : public pbl::Scene
{
public:
  GameEndMenuScene();

  void update(double delta) override;
  void render() override;
  pbl::SceneStackType getSceneStackType() const override { return pbl::SceneStackType::KEEP_UPDATE; }

private:
  void rebuildUI();

private:
  std::shared_ptr<pbl::ScreenResizeEventHandler> m_windowResizeEventHandler;
  pbl::GraphicalResourceRegistry m_resources;
  pbl::UIManager m_ui;
  std::vector<std::shared_ptr<pbl::ButtonElement>> m_buttons;
  int m_selectedButtonIndex{ -1 };
};
