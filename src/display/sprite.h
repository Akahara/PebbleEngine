#pragma once

#include "texture.h"
#include "renderable.h"
#include "shader.h"
#include "utils/debug.h"
#include "utils/regions.h"

namespace pbl
{

struct Sprite
{
  enum class SpriteType { DEFAULT, TEXT };

  Texture texture{};
  ScreenRegion region;
  float texX = 0, texY = 0, texW = 1, texH = 1; // texture region (bottom left 0:0 - top right 1:1)
  rvec4 color{ 1,1,1,1 };
  SpriteType spriteType = SpriteType::DEFAULT;

  Sprite() = default;
  Sprite(const Texture &tex, const ScreenRegion &region)
    : texture(tex), region(region) {}
  Sprite(const Texture &tex, const ScreenRegion &region, float uvx, float uvy, float uvw, float uvh)
    : texture(tex), region(region), texX(uvx), texY(uvy), texW(uvw), texH(uvh) {}
};

struct BillBoard {
  Texture texture{};
  vec3 position;
  rvec2 scale;
  float texX = 0, texY = 0, texW = 1, texH = 1; // texture region (bottom left 0:0 - top right 1:1)
};

class SpriteRenderer
{
public:
  explicit SpriteRenderer(GraphicalResourceRegistry &resources);
  // if an effect is null, this renderer won't be able to render that type of element
  SpriteRenderer(Effect *spriteEffect, Effect *billboardEffect);

  void clearSprites() { m_sprites.clear(); }
  void clearBillboards() { m_billboards.clear(); }
  void addSprite(const Sprite &sprite) {
    PBL_ASSERT(m_spriteEffect != nullptr, "No sprite effect specified, sprites cannot be rendered");
    m_sprites.push_back(sprite);
  }
  void addBillboard(const BillBoard &billboard) {
    PBL_ASSERT(m_billboardEffect != nullptr, "No billboard effect specified, billboards cannot be rendered");
    m_billboards.push_back(billboard);
  }

  std::vector<Sprite> &getSprites() {
    PBL_ASSERT(m_spriteEffect != nullptr, "No sprite effect specified, sprites cannot be rendered");
    return m_sprites;
  }

  std::vector<BillBoard> &getBillboards() {
    PBL_ASSERT(m_spriteEffect != nullptr, "No billboard effect specified, billboards cannot be rendered");
    return m_billboards;
  }

  void renderSprites(RenderContext &context);
  void renderBillboards(RenderContext &context);

  static const ShaderVertexLayout &getSpriteVertexLayout();
  static const ShaderVertexLayout &getBillboardVertexLayout();

private:
  static constexpr size_t DRAW_CALL_INSTANCE_CAPACITY = 150;
  Effect                     *m_spriteEffect;
  Effect                     *m_billboardEffect;
  GenericBuffer               m_screenConstantBuffer;
  GenericBuffer               m_billboardConstantBuffer;
  GenericBuffer               m_spriteInstanceBuffer;
  GenericBuffer               m_billboardInstanceBuffer;
  std::vector<Sprite>         m_sprites;
  std::vector<BillBoard>      m_billboards;
};

}