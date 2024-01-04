#include "settings_menu.h"

#include "game_scene.h"
#include "display/ui/ui_elements.h"
#include "inputs/user_inputs.h"
#include "scene/scene_manager.h"

using namespace pbl;

SettingsMenuScene::SettingsMenuScene()
  : m_ui(&m_resources)
{
  rebuildUI();
  Device::addWindowResizeEventHandler(m_windowResizeEventHandler = std::make_shared<ScreenResizeEventHandler>([this](int, int) { rebuildUI(); }));
}

void SettingsMenuScene::update(double delta)
{
  m_ui.update();
  m_ui.pollEvents();

  if (UserInputs::consumeKeyPress(keys::SC_ESCAPE) || UserInputs::consumeButtonPress(keys::SC_CIRCLE))
    SceneManager::getInstance().popLayeredScene();

  if (UserInputs::consumeDPadPress(keys::SC_DPAD_UP) || UserInputs::consumeKeyPress(keys::SC_W)) {
      if (m_selectedBoxIndex == -1) {
          m_checkboxes[0]->setSelected(true);
          m_selectedBoxIndex = 0;
      } else {
          m_checkboxes[m_selectedBoxIndex]->setSelected(false);
          m_selectedBoxIndex--;
          if (m_selectedBoxIndex == -1)
              m_selectedBoxIndex = m_checkboxes.size() - 1;
          m_checkboxes[m_selectedBoxIndex]->setSelected(true);
      }
  }
  if (UserInputs::consumeDPadPress(keys::SC_DPAD_DOWN) || UserInputs::consumeKeyPress(keys::SC_S)) {
      if (m_selectedBoxIndex == -1) {
          m_checkboxes[0]->setSelected(true);
          m_selectedBoxIndex = 0;
      } else {
          m_checkboxes[m_selectedBoxIndex]->setSelected(false);
          m_selectedBoxIndex = (m_selectedBoxIndex + 1) % m_checkboxes.size();
          m_checkboxes[m_selectedBoxIndex]->setSelected(true);
      }
  }

  if (UserInputs::consumeButtonPress(keys::SC_CROSS) || UserInputs::consumeKeyPress(keys::SC_ENTER))
      m_checkboxes[m_selectedBoxIndex]->triggerCallback();
}

void SettingsMenuScene::render()
{
  m_ui.render();
}

void SettingsMenuScene::rebuildUI()
{
  m_ui.clearUIElements();

  m_ui.addUIElement(std::make_shared<SpriteElement>(ScreenRegion::fullscreen(), m_resources.loadTexture(L"res/textures/screen_dark_filter.dds"), true));

  auto addOptionCheckbox = [&, y = ScreenRegion::SCREEN_HEIGHT*.6f](std::string_view text, bool &toggle) mutable {
    auto cb = std::make_shared<CheckboxElement>(
        ScreenPoint{ ScreenRegion::SCREEN_WIDTH*.5f, y -= CheckboxElement::BOX_SIZE*1.4f },
        text,
        [&](bool b) { toggle = b; },
        toggle
      );
    m_checkboxes.push_back(cb);
    m_ui.addUIElement(cb);
  };

  addOptionCheckbox("Bloom", GameVFX::getSettings().enableBloom);
  addOptionCheckbox("Screen shake", GameVFX::getSettings().enableScreenShake);
  addOptionCheckbox("Chromatic aberration", GameVFX::getSettings().enableChromaticAberration);
  addOptionCheckbox("Depth of field", GameVFX::getSettings().enableDepthOfField);
  addOptionCheckbox("Vignette", GameVFX::getSettings().enableVignette);
  addOptionCheckbox("Always tunnel vision", GameVFX::getSettings().alwaysEnableTunnelVision);

  addOptionCheckbox("Invert Y Axis", GameVFX::getSettings().invertYAxis);

  auto backBtn = std::make_shared<ButtonElement>("Back", .05f, .5f, Anchor::BOTTOM_LEFT);
  backBtn->setClickHandler([] { SceneManager::getInstance().popLayeredScene(); });
  m_ui.addUIElement(std::move(backBtn));
}
