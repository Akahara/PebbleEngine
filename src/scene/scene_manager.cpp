#include "scene_manager.h"

#include "transitions.h"
#include "utils/debug.h"

namespace pbl
{

SceneManager SceneManager::s_singleton;

void SceneManager::setActiveScene(std::unique_ptr<Scene> &&scene)
{
  PBL_ASSERT(m_sceneStack.empty(), "Called setActiveScene but a scene was still active!");
  m_sceneStack.push_back(std::move(scene));
  logs::scene.log("replaced the stack by a new scene");
}

void SceneManager::transitionToScene(SceneSupplier&& sceneSupplier, SceneTransition transition)
{
  pushLayeredScene(make_supplier<TransitionOutScene>(std::move(sceneSupplier), transition));
}

void SceneManager::pushLayeredScene(SceneSupplier &&sceneSupplier)
{
  m_transitionsActionQueue.emplace([this, sceneSupplier=std::move(sceneSupplier)] {
    if (!m_sceneStack.empty())
      m_sceneStack.back()->onSceneStateChange(SceneStateChange::LOST_FOCUS);
    m_sceneStack.push_back(sceneSupplier());
    m_sceneStack.back()->onSceneStateChange(SceneStateChange::AQUIRED_FOCUS);
    logs::scene.log("pushed a new scene onto the stack");
  });
}

void SceneManager::popLayeredScene()
{
  m_transitionsActionQueue.emplace([this] {
    m_sceneStack.back()->onSceneStateChange(SceneStateChange::LOST_FOCUS);
    m_sceneStack.pop_back();
    if (!m_sceneStack.empty())
      m_sceneStack.back()->onSceneStateChange(SceneStateChange::AQUIRED_FOCUS);
    logs::scene.log("deleted a scene from the top of the stack");
  });
}

void SceneManager::disposeAllScenes()
{
  m_transitionsActionQueue.emplace([this] {
    while (!m_sceneStack.empty()) {
      m_sceneStack.back()->onSceneStateChange(SceneStateChange::LOST_FOCUS);
      m_sceneStack.pop_back();
      if (!m_sceneStack.empty())
        m_sceneStack.back()->onSceneStateChange(SceneStateChange::AQUIRED_FOCUS);
    }
    logs::scene.log("deleted all scenes from the the stack");
  });
}

bool SceneManager::performTransitions()
{
  if (m_transitionsActionQueue.empty()) return false;
  while(!m_transitionsActionQueue.empty()) {
    m_transitionsActionQueue.front()();
    m_transitionsActionQueue.pop();
  }
  return true;
}

void SceneManager::update(double delta) const
{
  for(size_t i = m_sceneStack.size(); i > 0; i--) {
    m_sceneStack[i-1]->update(delta);
    if (m_sceneStack[i-1]->getSceneStackType() != SceneStackType::KEEP_UPDATE)
      break;
  }
}

void SceneManager::render() const
{
  for (const auto& i : m_sceneStack)
    i->render();
}

}
