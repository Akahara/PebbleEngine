#pragma once

#include "scene/scene.h"

#include "display/graphical_resource.h"
#include "display/ui/ui_elements.h"
#include "display/ui/ui_manager.h"
#include "engine/device.h"
#include <vector>

class PauseMenuScene : public pbl::Scene
{
public:
  PauseMenuScene();

  void update(double delta) override;
  void render() override;

private:
  void rebuildUI();

  void onSceneStateChange(pbl::SceneStateChange change) override
  {
    m_isSubMenuActive = change == pbl::SceneStateChange::LOST_FOCUS;
  }

private:
  std::shared_ptr<pbl::ScreenResizeEventHandler> m_windowResizeEventHandler;
  float m_sceneTime{};
  std::shared_ptr<pbl::TextElement> m_animatedText;
  pbl::GraphicalResourceRegistry m_resources;
  pbl::UIManager m_ui;
  bool m_isSubMenuActive = false;
  std::vector<std::shared_ptr<pbl::ButtonElement>> m_buttons;
  int m_selectedButtonIndex{ -1 };
};
