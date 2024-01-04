#include "end_menu.h"

#include "game_scene.h"
#include "main_menu.h"
#include "display/ui/ui_elements.h"
#include "scene/scene_manager.h"
#include "inputs/user_inputs.h"

using namespace pbl;

GameEndMenuScene::GameEndMenuScene()
  : m_ui(&m_resources)
{
  rebuildUI();
  Device::addWindowResizeEventHandler(m_windowResizeEventHandler = std::make_shared<ScreenResizeEventHandler>([this](int, int) { rebuildUI(); }));
}

void GameEndMenuScene::update(double delta)
{
  m_ui.update();
  m_ui.pollEvents();

  if (UserInputs::consumeDPadPress(keys::SC_DPAD_UP) || UserInputs::consumeKeyPress(keys::SC_W)) {
      if (m_selectedButtonIndex == -1) {
          m_buttons[0]->setSelected(true);
          m_selectedButtonIndex = 0;
      } else {
          m_buttons[m_selectedButtonIndex]->setSelected(false);
          m_selectedButtonIndex--;
          if (m_selectedButtonIndex == -1)
              m_selectedButtonIndex = m_buttons.size() - 1;
          m_buttons[m_selectedButtonIndex]->setSelected(true);
      }
  }
  if (UserInputs::consumeDPadPress(keys::SC_DPAD_DOWN) || UserInputs::consumeKeyPress(keys::SC_S)) {
      if (m_selectedButtonIndex == -1) {
          m_buttons[0]->setSelected(true);
          m_selectedButtonIndex = 0;
      } else {
          m_buttons[m_selectedButtonIndex]->setSelected(false);
          m_selectedButtonIndex = (m_selectedButtonIndex + 1) % m_buttons.size();
          m_buttons[m_selectedButtonIndex]->setSelected(true);
      }
  }

  if (UserInputs::consumeButtonPress(keys::SC_CROSS) || UserInputs::consumeKeyPress(keys::SC_ENTER))
      m_buttons[m_selectedButtonIndex]->triggerCallback();
}

void GameEndMenuScene::render()
{
  m_ui.render();
}

void GameEndMenuScene::rebuildUI()
{
  m_ui.clearUIElements();
  auto restartBtn = std::make_shared<ButtonElement>("Play again", 0.05f, 0.6f, Anchor::BOTTOM_LEFT);
  auto mainMenuBtn = std::make_shared<ButtonElement>("Back to menu", 0.05f, 0.5f, Anchor::BOTTOM_LEFT);
  m_buttons.push_back(restartBtn);
  m_buttons.push_back(mainMenuBtn);
  restartBtn->setTextScale(.045f);
  mainMenuBtn->setTextScale(.045f);
  restartBtn->setClickHandler([] { SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<GameScene>()); });
  mainMenuBtn->setClickHandler([] { SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<MainMenuScene>()); });
  m_ui.addUIElement(std::make_shared<SpriteElement>(ScreenRegion::fullscreen(), m_resources.loadTexture(L"res/textures/screen_dark_filter.dds"), true));
  m_ui.addUIElement(std::move(restartBtn));
  m_ui.addUIElement(std::move(mainMenuBtn));
}
