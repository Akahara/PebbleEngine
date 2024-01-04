#include "ui_elements.h"

#include "inputs/user_inputs.h"
#include "utils/regions.h"

namespace pbl
{

REGISTER_TEXTURE_ENTRY(CheckboxChecked, L"res/sprites/checkbox_checked.dds");
REGISTER_TEXTURE_ENTRY(CheckboxUnchecked, L"res/sprites/checkbox_unchecked.dds");

void ContainerElement::setPosition(float x, float y)
{
  m_region = ScreenRegion::moveRegionCorner(m_region, x, y, m_anchor);
}

void ContainerElement::setWidth(float w)
{
  switch (m_anchor & Anchor::MASK_HORIZONTAL) {
  case Anchor::LEFT:
    m_region.maxX = m_region.minX + w;
    break;
  case Anchor::HMIDDLE:
    m_region.minX = (m_region.minX + m_region.maxX)/2.f - w/2.f;
    m_region.maxX = (m_region.minX + m_region.maxX)/2.f + w/2.f;
    break;
  case Anchor::RIGHT:
    m_region.minX = m_region.maxX - w;
    break;
  default:
    break;
  }
}

void ContainerElement::setHeight(float h)
{
  switch (m_anchor & Anchor::MASK_VERTICAL) {
  case Anchor::TOP:
    m_region.minY = m_region.maxY - h;
    break;
  case Anchor::VMIDDLE:
    m_region.minY = (m_region.minY + m_region.maxY) / 2.f - h/2.f;
    m_region.maxY = (m_region.minY + m_region.maxY) / 2.f + h/2.f;
    break;
  case Anchor::BOTTOM:
    m_region.maxY = m_region.minY + h;
    break;
  default:
    break;
  }
}

void ContainerElement::render(ScreenRegion screenRegion)
{
  if (!m_active) return;
  ScreenRegion region = screenRegion.regionInRegion(m_region);
  if (!m_background.empty()) 
    m_ui->getSprites().addSprite(Sprite{ m_background, region });
  for (auto &ue : m_content)
    ue->render(region);
}

void TextElement::render(ScreenRegion screenRegion) // FIX! screenRegion not considered in text rendering (because ScreenRegion::xyInRegion is broken)
{
  if (!m_active) return;
  m_ui->getText()
    .setColor(m_textColor)
    .drawText(m_text, m_textRegion.minX, m_textRegion.minY, m_textScale);
}

void TextElement::updateTextRegion()
{
  if (!m_ui) return;
  ScreenRegion baseRegion = m_ui->getText().getTextRegion(m_text, 0, 0, m_textScale);
  m_textRegion = ScreenRegion::moveRegionCorner(baseRegion, m_x, m_y, m_textAnchor);
}

ScreenRegion ButtonElement::getInteractiveRegion() const
{
  return m_textRegion;
}

EventResponse ButtonElement::onUserClick(ScreenPoint mousePosition)
{
  if (!m_clickCallback || !m_active) return EventResponse::IGNORED;
  m_clickCallback();
  return EventResponse::CONSUMED;
}

void ButtonElement::render(ScreenRegion screenRegion)
{
  if (!m_active) return;
  m_textColor = m_textRegion.contains(UserInputs::getMousePosition()) || m_selected ? rvec4{0.5,0.2,0.5,1} : rvec4{.5f,.5f,.5f,1.f};
  TextElement::render(screenRegion);
}

void SpriteElement::render(ScreenRegion screenRegion)
{
	if (!m_texture.empty())
		m_ui->getSprites().addSprite({ m_texture, screenRegion.regionInRegion(m_region) });
}

CheckboxElement::CheckboxElement(ScreenPoint position, std::string_view text, std::function<void(bool)> onCheck, bool initialyChecked)
  : m_checked(initialyChecked)
  , m_text(text, position.x + BOX_SIZE, position.y + BOX_SIZE*.15f, BOX_SIZE, Anchor::VMIDDLE|Anchor::LEFT)
  , m_onCheck(std::move(onCheck))
  , m_boxRegion{ position.x-BOX_SIZE*.5f, position.y-BOX_SIZE*.5f, position.x+BOX_SIZE*.5f, position.y+BOX_SIZE*.5f }
{
}

void CheckboxElement::render(ScreenRegion screenRegion)
{
  m_ui->getSprites().addSprite(Sprite{ m_checked ? m_ui->getTexture<CheckboxChecked>() : m_ui->getTexture<CheckboxUnchecked>(), m_boxRegion });
  m_text.setTextColor(m_interactibleRegion.contains(UserInputs::getMousePosition()) || m_selected ? rvec4{1,1,1,1} : rvec4{.8f,.8f,.8f,1.f});
  m_text.render(screenRegion);
}

EventResponse CheckboxElement::onUserClick(ScreenPoint mousePosition)
{
  m_checked = !m_checked;
  m_onCheck(m_checked);
  return EventResponse::CONSUMED;
}

void CheckboxElement::setUIManager(UIManager* ui)
{
  UIElement::setUIManager(ui);
  ui->registerTexture<CheckboxChecked>();
  ui->registerTexture<CheckboxUnchecked>();
  static_cast<UIElement&>(m_text).setUIManager(ui);
  m_interactibleRegion = ScreenRegion::unionRegion(m_text.getTextRegion(), m_boxRegion);
}

}
