#include "engine.h"

#include "utils/debug.h"
#include "inputs/user_inputs.h"
#include "display/frame_buffer.h"
#include "display/graphical_resource.h"
#include "display/render_profiles.h"
#include "display/mesh.h"
#include "display/skybox.h"
#include "display/renderer.h"
#include "world/object.h"
#include "physics/physics.h"
#include "scene/scene_manager.h"
#include "scene/editor/editor_scene.h"
#include "scene/game/main_menu.h"
#include "scene/game/game_scene.h"

namespace pbl
{

static constexpr double FRAME_DELTA = 1./60.;

EngineBase::EngineBase(EngineSettings settings)
  : m_settings(std::move(settings))
{}

EngineBase::~EngineBase()
{
  m_primaryFrameBuffer->unbind();
}

void EngineBase::init()
{
  m_device = createDeviceSpecific(DeviceMode::WINDOWED);
  m_previousTime = m_clock.getTimeAsCount();
  m_globalGraphicalResources = std::make_unique<GraphicalResourceRegistry>();

  renderer::loadGlobalResources(*m_globalGraphicalResources);
  RenderProfiles::initProfiles();
  Mesh::loadGlobalResources();
  WorldObject::loadGlobalResources();
  Skybox::loadGlobalResources(*m_globalGraphicalResources);
  FrameBufferPipeline::loadGlobalResources(*m_globalGraphicalResources);
  Renderable::loadGlobalResources(*m_globalGraphicalResources);
  pbx::Physics::loadGlobalResources();

  m_primaryFrameBuffer = std::make_unique<FrameBuffer>(Device::getWinWidth(), Device::getWinHeight(), FrameBuffer::Target::COLOR_0 | FrameBuffer::Target::DEPTH_STENCIL);
  m_primaryFrameBuffer->bind();

  Device::addWindowResizeEventHandler(m_windowResizeEventHandler = std::make_shared<ScreenResizeEventHandler>([this](int w, int h) {
	  m_primaryFrameBuffer->unbind();
	  m_primaryFrameBuffer = std::make_unique<FrameBuffer>(w, h, FrameBuffer::Target::COLOR_0 | FrameBuffer::Target::DEPTH_STENCIL);
	  m_primaryFrameBuffer->bind();
  }));
}

void EngineBase::dispose()
{
  SceneManager::getInstance().disposeAllScenes();
  SceneManager::getInstance().performTransitions();
  UserInputs::unloadGlobalResources();
  Renderable::unloadGlobalResources();
  pbx::Physics::unloadGlobalResources();
  FrameBufferPipeline::unloadGlobalResources();
  Skybox::unloadGlobalResources();
  WorldObject::unloadGlobalResources();
  Mesh::unloadGlobalResources();
  RenderProfiles::disposeProfiles();
  renderer::unloadGlobalResources();
}

void EngineBase::run()
{
  SceneManager::getInstance().setActiveScene(std::make_unique<MainMenuScene>());

  bool running = true;
  while (running && !shouldExit) {
	  running &= pollEvents();
	  if (!running) return;

	  int64_t currentTime = m_clock.getTimeAsCount();
	  double delta = m_clock.getDeltaSeconds(m_previousTime, currentTime);
	  if (delta <= FRAME_DELTA) continue;

	  running &= postFrame();
	  running &= runFrame(delta);

	  // if scenes need to be created/deleted drop the frames instead of trying to catch back
	  if (SceneManager::getInstance().performTransitions())
		  m_previousTime = m_clock.getTimeAsCount();
	  else
		  m_previousTime = currentTime;
  }
}

void EngineBase::exit()
{
	shouldExit = true;
}

bool EngineBase::runFrame(double delta)
{
  // update
  UserInputs::pollEvents();
  SceneManager::getInstance().update(delta);
  
  // render
  beginRenderSceneSpecific();
  SceneManager::getInstance().render();
#ifdef PBL_ISDEBUG
  void renderScenesMenu();
  renderScenesMenu();
#endif

  LogsWindow::renderWindows();
  endRenderSceneSpecific();

  return true;
}

bool EngineBase::postFrame()
{
  m_device->getBackbuffer().bind();
  FrameBufferPipeline::doSimpleBlit(m_primaryFrameBuffer->getTargetAsTexture(FrameBuffer::COLOR_0));
  Effect::unbindResources();
  m_device->getBackbuffer().unbind();
  m_device->present();
  return true;
}

}
