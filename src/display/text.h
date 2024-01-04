#pragma once

#include "sprite.h"

namespace pbl
{

struct Character {
  int codePoint, x, y, width, height, originX, originY, advance;
};

struct Font {
  const char *name;
  int size, bold, italic, width, height, characterCount;
  Character *characters;
};

namespace fonts
{
extern const Font &defaultFont;
}


class TextRenderer {
public:
  TextRenderer(const Font &font, GraphicalResourceRegistry &resources, SpriteRenderer *spriteRenderer);

  // xy are in screen space
  // a scale of 1 makes a single line use the full height of the screen
  TextRenderer &drawText(std::string_view text, float x, float y, float lineHeight);
  ScreenRegion getTextRegion(std::string_view text, float x, float y, float lineHeight) const;

  TextRenderer &setColor(const rvec4 &color)
  {
    m_textColor = color;
    return *this;
  }

private:
  const Character &getChar(char c) const;
  static ScreenRegion getCharPositionOnScreen(const Character &c, float currentX, float currentY, float scale);

private:
  const Font     *m_font;
  Texture         m_charactersSheetTexture;
  SpriteRenderer *m_spriteRenderer;
  rvec4           m_textColor{ 1,1,1,1 };
};

}
