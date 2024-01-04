#include "text.h"

#include "display/graphical_resource.h"

namespace pbl
{

namespace fonts
{

// https://evanw.github.io/font-texture-generator/
// with patch https://github.com/evanw/font-texture-generator/pull/2/files
#include "generated_fonts.h"
const Font &defaultFont = font_RubikMonoOne;

}

static constexpr char FONT_SHEET_FIRST_CHAR = ' ';
static constexpr char FONT_SHEET_LAST_CHAR = '~';


TextRenderer::TextRenderer(const Font &font, GraphicalResourceRegistry &resources, SpriteRenderer *spriteRenderer)
  : m_font(&font)
  , m_spriteRenderer(spriteRenderer)
  , m_charactersSheetTexture(resources.loadTexture(L"res/fonts/" + utils::string2widestring(font.name) + L".dds"))
{
}

TextRenderer &TextRenderer::drawText(std::string_view text, float x, float y, float lineHeight)
{
  float currentX = x, currentY = y;
  for (char ch : text) {
    if (ch == '\n') {
      currentX = x;
      currentY -= lineHeight;
      continue;
    } else if (ch < FONT_SHEET_FIRST_CHAR || ch > FONT_SHEET_LAST_CHAR) {
      ch = '?';
    }

    const Character &c = getChar(ch);

    Sprite charSprite{};
    charSprite.texture = m_charactersSheetTexture;
    charSprite.region = getCharPositionOnScreen(c, currentX, currentY, lineHeight / m_font->size);
    charSprite.color  = m_textColor;
    charSprite.texX   = (float)c.x / m_font->width;
    charSprite.texW   = (float)c.width / m_font->width;
    charSprite.texY   = (float)c.y / m_font->height;
    charSprite.texH   = (float)c.height / m_font->height;
    charSprite.spriteType = Sprite::SpriteType::TEXT;
    m_spriteRenderer->addSprite(charSprite);

    currentX += c.advance * lineHeight / m_font->size;
  }

  return *this;
}

ScreenRegion TextRenderer::getTextRegion(std::string_view text, float x, float y, float lineHeight) const
{
  ScreenRegion region{ x, y, x, y+lineHeight };

  float currentX = x, currentY = y;
  for (char ch : text) {
    if (ch == '\n') {
      currentX = x;
      currentY -= lineHeight;
      region.minY = std::min(region.minY, currentY);
      continue;
    } else if (ch < FONT_SHEET_FIRST_CHAR || ch > FONT_SHEET_LAST_CHAR) {
      ch = '?';
    }
    const Character &c = getChar(ch);
    currentX += c.advance * lineHeight / m_font->size;
    region.maxX = std::max(region.maxX, currentX);
  }
  
  return region;
}

const Character &TextRenderer::getChar(char c) const
{
  return m_font->characters[c - FONT_SHEET_FIRST_CHAR];
}

ScreenRegion TextRenderer::getCharPositionOnScreen(const Character &c, float currentX, float currentY, float scale)
{
  ScreenRegion region;
  region.minX = currentX - c.originX * scale;
  region.minY = currentY - (c.height - c.originY) * scale;
  region.maxX = region.minX + c.width * scale;
  region.maxY = region.minY + c.height * scale;
  return region;
}
}
