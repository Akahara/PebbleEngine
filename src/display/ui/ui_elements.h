#pragma once

#include <functional>
#include <string>

#include "ui_manager.h"

namespace pbl
{
 
class ContainerElement : public UIElement {
public:
    explicit ContainerElement(float min_x = 0, float min_y = 0, float max_x = 1, float max_y = 1)
      : m_anchor(Anchor::BOTTOM_LEFT)
      , m_region(min_x, min_y, max_x, max_y)
    {}

    void render(ScreenRegion screenRegion) override;

    void setPosition(float x, float y);
    void setWidth(float w);
    void setHeight(float h);
    void setAnchor(anchor_t anchor) { m_anchor = anchor; }

    void setBackground(const Texture &backgroundTexture) 
    {
      m_background = backgroundTexture;
    }

    void addUIElement(const std::shared_ptr<UIElement> &element)
    { 
      element->setUIManager(m_ui);
      m_content.push_back(element); 
    }

protected:
    ScreenRegion m_region;
    Texture      m_background;
    anchor_t     m_anchor;
    std::vector<std::shared_ptr<UIElement>> m_content;
};

class TextElement : public UIElement {
public:
  explicit TextElement(std::string_view text, float x=0, float y=0, float textScale=.05f, anchor_t anchor=Anchor::CENTER)
    : m_text(text), m_x(x), m_y(y), m_textScale(textScale), m_textAnchor(anchor)
  {}

  void render(ScreenRegion screenRegion) override;

  void setPosition(float x, float y) { m_x = x; m_y = y; updateTextRegion(); }
  void setText(const std::string &text) { m_text = text; updateTextRegion(); }
  void setTextScale(float scale) { m_textScale = scale; updateTextRegion(); }
  void setAnchor(anchor_t anchor) { m_textAnchor = anchor; updateTextRegion(); }
  void setTextColor(const rvec4 &color) { m_textColor = color; }
  const ScreenRegion &getTextRegion() const { return m_textRegion; }

protected:
  void updateTextRegion();

  void setUIManager(UIManager* ui) override {
    UIElement::setUIManager(ui);
    updateTextRegion();
  }

protected:
  std::string m_text;
  float m_x, m_y;
  anchor_t m_textAnchor;
  float m_textScale;
  rvec4 m_textColor = { 1,1,1,1 };
  ScreenRegion m_textRegion;
};

class ButtonElement : public TextElement {
public:
  explicit ButtonElement(std::string text = {}, float x = 0, float y = 0, anchor_t anchor = Anchor::CENTER)
    : TextElement(std::move(text), x, y, .05f, anchor) {}

  ScreenRegion getInteractiveRegion() const override;
  EventResponse onUserClick(ScreenPoint mousePosition) override;
  void render(ScreenRegion screenRegion) override;
  void triggerCallback() { m_clickCallback(); }
  void setSelected(bool b) { m_selected = b; }

  void setClickHandler(const std::function<void()>& callback) { m_clickCallback = callback; }

private:
  std::function<void()> m_clickCallback;
  bool m_selected{ false };
};

class SpriteElement : public UIElement {
public:
  explicit SpriteElement(ScreenRegion region, const Texture &texture, bool blockMouseEvents = false)
    : m_region(region)
    , m_blockMouseEvents(blockMouseEvents)
    , m_texture(texture)
  {}

  void setTexture(const Texture& texture) { m_texture = texture; }

  void render(ScreenRegion screenRegion) override;
  ScreenRegion getInteractiveRegion() const override { return m_blockMouseEvents ? m_region : ScreenRegion{}; }
  EventResponse onUserClick(ScreenPoint mousePosition) override { return EventResponse::CONSUMED; }

private:
  bool         m_blockMouseEvents;
  ScreenRegion m_region;
  Texture      m_texture;
};

class CheckboxElement : public UIElement {
public:
  static constexpr float BOX_SIZE = .04f;

  explicit CheckboxElement(ScreenPoint position, std::string_view text, std::function<void(bool)> onCheck, bool initialyChecked=false);

  void render(ScreenRegion screenRegion) override;
  ScreenRegion getInteractiveRegion() const override { return m_interactibleRegion; }
  EventResponse onUserClick(ScreenPoint mousePosition) override;
  void triggerCallback() { m_checked = !m_checked; m_onCheck(m_checked); }
  void setSelected(bool b) { m_selected = b; }
  
  void setUIManager(UIManager *ui) override;

private:
  ScreenRegion m_boxRegion;
  ScreenRegion m_interactibleRegion;
  TextElement m_text;
  bool m_checked;
  std::function<void(bool)> m_onCheck;
  bool m_selected{ false };
};

}
