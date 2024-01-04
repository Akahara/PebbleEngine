#include "pause_menu.h"

#include "game_scene.h"
#include "main_menu.h"
#include "settings_menu.h"
#include "display/ui/ui_elements.h"
#include "inputs/user_inputs.h"
#include "scene/scene_manager.h"

using namespace pbl;

static constexpr const char *ANIMATED_TEXTS[] = {
  "Game Paused   ",
  "Game Paused.  ",
  "Game Paused.. ",
  "Game Paused...",
};

PauseMenuScene::PauseMenuScene()
  : m_ui(&m_resources)
{
  rebuildUI();
  Device::addWindowResizeEventHandler(m_windowResizeEventHandler = std::make_shared<ScreenResizeEventHandler>([this](int, int) { rebuildUI(); }));
}

void PauseMenuScene::update(double delta)
{
  m_sceneTime += static_cast<float>(delta);

  m_animatedText->setText(ANIMATED_TEXTS[static_cast<int>(m_sceneTime) % std::size(ANIMATED_TEXTS)]);

  m_ui.update();
  m_ui.pollEvents();

  if (UserInputs::consumeKeyPress(keys::SC_ESCAPE) || UserInputs::consumeButtonPress(keys::SC_CIRCLE))
    SceneManager::getInstance().popLayeredScene();

  if (UserInputs::consumeDPadPress(keys::SC_DPAD_UP) || UserInputs::consumeKeyPress(keys::SC_W))
  {
      if (m_selectedButtonIndex == -1)
      {
          m_buttons[0]->setSelected(true);
          m_selectedButtonIndex = 0;
      }
      else 
      {
          m_buttons[m_selectedButtonIndex]->setSelected(false);
          m_selectedButtonIndex--;
          if (m_selectedButtonIndex == -1)
              m_selectedButtonIndex = m_buttons.size() - 1;
          m_buttons[m_selectedButtonIndex]->setSelected(true);
      }
  }
  if (UserInputs::consumeDPadPress(keys::SC_DPAD_DOWN) || UserInputs::consumeKeyPress(keys::SC_S))
  {
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

void PauseMenuScene::render()
{
  if(!m_isSubMenuActive)
    m_ui.render();
}

void PauseMenuScene::rebuildUI()
{
  m_ui.clearUIElements();

  m_ui.addUIElement(std::make_shared<SpriteElement>(ScreenRegion::fullscreen(), m_resources.loadTexture(L"res/textures/screen_dark_filter.dds"), true));
  m_animatedText = std::make_shared<TextElement>(ANIMATED_TEXTS[0], ScreenRegion::SCREEN_WIDTH*.5f, ScreenRegion::SCREEN_HEIGHT*.95f, .05f, Anchor::TOP|Anchor::HMIDDLE);
  m_ui.addUIElement(m_animatedText);

  auto continueBtn = std::make_shared<ButtonElement>("Continue", .05f, .6f, Anchor::BOTTOM_LEFT);
  continueBtn->setClickHandler([] { SceneManager::getInstance().popLayeredScene(); });
  m_buttons.push_back({continueBtn});
  m_ui.addUIElement(std::move(continueBtn));

  auto settingsBtn = std::make_shared<ButtonElement>("Settings", .05f, .5f, Anchor::BOTTOM_LEFT);
  settingsBtn->setClickHandler([] { SceneManager::getInstance().pushLayeredScene(SceneManager::make_supplier<SettingsMenuScene>()); });
  m_buttons.push_back({ settingsBtn });
  m_ui.addUIElement(std::move(settingsBtn));

  auto backBtn = std::make_shared<ButtonElement>("Exit Track", .05f, .4f, Anchor::BOTTOM_LEFT);
  backBtn->setClickHandler([] { SceneManager::getInstance().transitionToScene(SceneManager::make_supplier<MainMenuScene>()); });
  m_buttons.push_back({ backBtn });
  m_ui.addUIElement(std::move(backBtn));
}
