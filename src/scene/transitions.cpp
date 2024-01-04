#include "transitions.h"

#include "scene_manager.h"

namespace pbl
{

#ifdef PBL_ISDEBUGA
static constexpr double TRANSITION_OUT_DURATION = .05f;
static constexpr double TRANSITION_IN_DURATION = .05f;
#else
static constexpr double TRANSITION_OUT_DURATION = .6f;
static constexpr double TRANSITION_IN_DURATION = .4f;
#endif

struct TransitionConstantBuffer {
  float vTransitionTime; // 0..1 with .5 being completely black
  char __padding[4*3]{};
};

TransitionInScene::TransitionInScene()
  : m_sprites{ m_resources.loadEffect(L"res/shaders/scene_transition.fx", SpriteRenderer::getSpriteVertexLayout()), nullptr }
  , m_effectCB(GenericBuffer::make_buffer<TransitionConstantBuffer>(GenericBuffer::BUFFER_CONSTANT))
{
  m_sprites.addSprite(Sprite{ m_resources.loadTexture(L"res/sprites/transition.dds"), ScreenRegion::fullscreen() });
}

void TransitionInScene::render()
{
  TransitionConstantBuffer cbData{ static_cast<float>(std::min(m_time, 1.))*.5f+.5f };
  m_effectCB->setData(cbData);
  RenderContext ctx;
  ctx.constantBufferBindings.push_back({ "cbTransition", m_effectCB.get() });
  m_sprites.renderSprites(ctx);
}

void TransitionInScene::update(double delta)
{
  m_time += delta / TRANSITION_IN_DURATION;
  if (m_time > 1)
    SceneManager::getInstance().popLayeredScene();
}

TransitionOutScene::TransitionOutScene(std::function<std::unique_ptr<Scene>()> &&nextSceneSupplier, SceneTransition transition)
  : m_followingScene(std::move(nextSceneSupplier))
  , m_sprites{ m_resources.loadEffect(L"res/shaders/scene_transition.fx", SpriteRenderer::getSpriteVertexLayout()), nullptr }
  , m_effectCB(GenericBuffer::make_buffer<TransitionConstantBuffer>(GenericBuffer::BUFFER_CONSTANT))
{
  m_sprites.addSprite(Sprite{ m_resources.loadTexture(L"res/sprites/transition.dds"), ScreenRegion::fullscreen() });

  if (transition == SceneTransition::INSTANT_OUT)
    m_time = 1;
}

void TransitionOutScene::render()
{
  TransitionConstantBuffer cbData{ static_cast<float>(std::min(m_time, 1.))*.5f };
  m_effectCB->setData(cbData);
  RenderContext ctx;
  ctx.constantBufferBindings.push_back({ "cbTransition", m_effectCB.get() });
  m_sprites.renderSprites(ctx);
}

void TransitionOutScene::update(double delta)
{
  m_time += delta / TRANSITION_OUT_DURATION;
  if (m_time >= 1) {
    m_time = 1;
    if(m_hasRenderedOpaqueFrame) {
      SceneManager::getInstance().disposeAllScenes();
      SceneManager::getInstance().pushLayeredScene(std::move(m_followingScene));
      SceneManager::getInstance().pushLayeredScene([] { return std::unique_ptr<TransitionInScene>{ new TransitionInScene }; });
    } else {
      m_hasRenderedOpaqueFrame = true;
    }
  }
}

}
