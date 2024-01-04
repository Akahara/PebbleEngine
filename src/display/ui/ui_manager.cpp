#include "ui_manager.h"

#include "display/graphical_resource.h"
#include "inputs/user_inputs.h"

namespace pbl
{

UIManager::UIManager(GraphicalResourceRegistry *resources)
  : m_resources(resources)
  , m_sprites(*resources)
  , m_text(fonts::defaultFont, *resources, &m_sprites)
{
}

void UIManager::addUIElement(std::shared_ptr<UIElement>&& element)
{
  element->setUIManager(this);
  m_uiElements.push_back(element);
}

void UIManager::clearUIElements()
{
  m_uiElements.clear();
}

void UIManager::render()
{
  ScreenRegion screen = ScreenRegion::fullscreen();
  for (auto &el : m_uiElements)
    el->render(screen);
  RenderContext ctx;
  m_sprites.renderSprites(ctx);
  m_sprites.clearSprites();
}

void UIManager::update()
{
  for (auto &el : m_uiElements)
    el->update();
}

void UIManager::pollEvents()
{
  // process click events
  ScreenPoint mousePosition = UserInputs::getMousePosition();
  if(UserInputs::consumeClick(MouseState::BUTTON_PRIMARY)) {
    bool eventConsumed = false;
    for (size_t i = 0; i < m_uiElements.size(); i++) {
      auto &el = m_uiElements[m_uiElements.size()-1-i];
      if(el->getInteractiveRegion().contains(mousePosition)) {
        switch (el->onUserClick(mousePosition))
        {
        case EventResponse::CONSUMED:
          eventConsumed = true;
          break;
        case EventResponse::IGNORED:
          continue;
        case EventResponse::PASSTHROUGH:
          eventConsumed = true;
          break;
        }
      }
    }
    //event_processed:
    if (eventConsumed) {
        UserInputs::consumeClick(MouseState::BUTTON_PRIMARY);
    }
  }
}

Texture UIManager::registerTexture(const std::string& name, const std::wstring& filepath)
{
  if (m_sharedTextures.contains(name))
    return m_sharedTextures[name];
  Texture tex = m_resources->loadTexture(filepath);
  return m_sharedTextures[name] = tex;
}

}
