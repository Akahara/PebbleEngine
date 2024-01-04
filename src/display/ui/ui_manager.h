#pragma once

#include <vector>

#include "display/sprite.h"
#include "display/text.h"
#include "utils/regions.h"

#define REGISTER_TEXTURE_ENTRY(name, path) struct name { inline static std::string entryName = #name; inline static std::wstring filepath = path; };

namespace pbl
{

enum class EventResponse {
  CONSUMED,
  IGNORED,
  PASSTHROUGH,
};

class UIManager;

class UIElement {
public:
  UIElement() = default;
  virtual ~UIElement() = default;

  UIElement(const UIElement &) = delete;
  UIElement &operator=(const UIElement &) = delete;
  UIElement(UIElement &&) = delete;
  UIElement &operator=(UIElement &&) = delete;

  virtual void render(ScreenRegion screenRegion) = 0;
  virtual void update() {}
  virtual ScreenRegion getInteractiveRegion() const { return {}; }
  // currently the only supported event
  virtual EventResponse onUserClick(ScreenPoint mousePosition) { return EventResponse::IGNORED; }

  // may be overriden to add ui initialization (ie. texture loading)
  virtual void setUIManager(UIManager *ui) { m_ui = ui; }

  virtual void setActive(bool active) { m_active = active; }

protected:
  UIManager *m_ui = nullptr;
  bool m_active = true;
};

class UIManager
{
public:
  explicit UIManager(GraphicalResourceRegistry *resources);

  void addUIElement(std::shared_ptr<UIElement> &&element);
  void clearUIElements();

  void render();
  void update();
  void pollEvents();

  SpriteRenderer &getSprites() { return m_sprites; }
  TextRenderer &getText() { return m_text; }

  Texture registerTexture(const std::string &name, const std::wstring &filepath);
  Texture getTexture(const std::string &name) const
    { return m_sharedTextures.at(name); }

  template<class TextureEntry>
  Texture registerTexture() { return registerTexture(TextureEntry::entryName, TextureEntry::filepath); }
  template<class TextureEntry>
  Texture getTexture() { return getTexture(TextureEntry::entryName); }

private:
  GraphicalResourceRegistry *m_resources;
  SpriteRenderer m_sprites;
  TextRenderer m_text;
  std::vector<std::shared_ptr<UIElement>> m_uiElements;
  std::unordered_map<std::string, Texture> m_sharedTextures;
};

}
