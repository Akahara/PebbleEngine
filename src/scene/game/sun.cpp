#include "sun.h"

#include "display/frame_buffer.h"
#include "display/graphical_resource.h"

static constexpr float SUN_SPEED = .05f; // radiants per second

Sun::Sun(pbl::GraphicalResourceRegistry &resources, pbl::SpriteRenderer *sprites)
  : m_sprites(sprites)
  , m_texture(resources.loadTexture(L"res/textures/sun.dds"))
  , m_direction()
  , m_depthFBO(1024, 1024, pbl::FrameBuffer::Target::DEPTH_STENCIL)
  , m_worldConstantBuffer(sizeof(pbl::WorldUniforms), pbl::GenericBuffer::BUFFER_CONSTANT)
{
  pbl::OrthographicProjection proj;
  proj.zNear = -10;
  proj.zFar = +10;
  proj.width = 10;
  proj.height = 10;
  m_camera.setProjection(proj);
}

void Sun::update(double delta)
{
  m_angle += static_cast<float>(delta) * SUN_SPEED;
  m_direction = DirectX::XMVector3Normalize({ std::sin(m_angle), -1, std::cos(m_angle) });
}

void Sun::render(pbl::RenderContext &context)
{
  m_sprites->addBillboard({ m_texture, context.camera.getPosition() - m_direction * WORLD_DISTANCE, {WORLD_SIZE*WORLD_DISTANCE,WORLD_SIZE*WORLD_DISTANCE} });
}

pbl::Texture Sun::renderDepthTexture(const std::vector<std::shared_ptr<WorldObject>> &shadowCastingObjects, vec3 primaryTarget)
{
  m_camera.setPosition(primaryTarget);
  m_camera.lookAt(primaryTarget+m_direction);
  m_camera.updateViewMatrix();
  pbl::WorldUniforms worldUniforms{};
  worldUniforms.vWorldMatViewProj = XMMatrixTranspose(m_camera.getViewProjectionMatrix());
  m_worldConstantBuffer.setData(worldUniforms);

  pbl::Effect::unbindResources();
  pbl::RenderContext worldRenderContext(m_camera);
  worldRenderContext.constantBufferBindings.push_back({ "cbWorld", &m_worldConstantBuffer });

  m_depthFBO.clearTargets();
  m_depthFBO.bind();
  for (auto &object : shadowCastingObjects)
    object->renderShadows(worldRenderContext);
  m_depthFBO.unbind();
  return m_depthFBO.getTargetAsTexture(pbl::FrameBuffer::DEPTH_STENCIL);
}
