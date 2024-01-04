#include "graphical_resource.h"

#include "directxlib.h"
#include "utils/debug.h"

namespace pbl
{

GraphicalResourceRegistry::~GraphicalResourceRegistry()
{
  for (auto &[_, texture] : m_texturesCache) {
    texture.m_resource->Release();
    texture.m_texture->Release();
  }
  for (auto &[_, cubemap] : m_cubemapsCache) {
    cubemap.m_resource->Release();
    cubemap.m_texture->Release();
  }
  for (auto &[_, effect] : m_effects) {
    DXRelease(effect.first.m_effect);
    DXRelease(effect.first.m_inputLayout);
  }
  // meshes are released on deletion
}

Texture GraphicalResourceRegistry::loadTexture(const filepath &path)
{
  if (m_texturesCache.contains(path))
    return m_texturesCache[path];
  return m_texturesCache[path] = TextureManager::loadTexture(path);
}

Cubemap GraphicalResourceRegistry::loadCubemap(const filepath &path)
{
  if (m_cubemapsCache.contains(path))
    return m_cubemapsCache[path];
  return m_cubemapsCache[path] = TextureManager::loadCubemap(path);
}

std::shared_ptr<Mesh> GraphicalResourceRegistry::loadMesh(const filepath &path)
{
  if (m_meshes.contains(path))
    return m_meshes[path];
  return m_meshes[path] = std::make_shared<Mesh>(ModelLoader::loadMesh(utils::widestring2string(path).c_str(), *this));
}

std::shared_ptr<Model> GraphicalResourceRegistry::loadModel(const filepath& path)
{
  if (m_models.contains(path))
    return m_models[path];
  return m_models[path] = std::make_shared<Model>(ModelLoader::loadModel(utils::widestring2string(path).c_str()));
}

std::pair<std::shared_ptr<Model>, std::shared_ptr<Mesh>> GraphicalResourceRegistry::loadModelMesh(const filepath& path)
{
  bool knownModel = m_models.contains(path);
  bool knownMesh = m_meshes.contains(path);
  if (knownMesh && knownModel)
    return { m_models[path], m_meshes[path] };
  if (knownMesh)
    return { loadModel(path), m_meshes[path] };
  if (knownModel)
    return { m_models[path], loadMesh(path) };
  auto [model, mesh] = ModelLoader::loadModelMesh(utils::widestring2string(path).c_str(), this);
  return {
    m_models[path] = std::make_shared<Model>(std::move(model)),
    m_meshes[path] = std::make_shared<Mesh> (std::move(mesh )),
  };
}

void GraphicalResourceRegistry::reloadShaders()
{
  for(auto &[file, oldEffect] : m_effects) {
    try {
      Effect newEffect = ShaderManager::makeEffect(file, oldEffect.second);
      DXRelease(oldEffect.first.m_effect);
      oldEffect.first.m_constantBufferBindingsCache.clear();
      oldEffect.first.m_variableBindingsCache.clear();
      oldEffect.first.m_pass = newEffect.m_pass;
      oldEffect.first.m_effect = newEffect.m_effect;
      oldEffect.first.m_technique = newEffect.m_technique;
    } catch (const std::runtime_error &e) {
      logs::graphics.logm(utils::widestring2string(file), " compilation error: ", e.what());
    }
  }
}

Effect *GraphicalResourceRegistry::loadEffect(const filepath &path, const ShaderVertexLayout &layout)
{
  if (m_effects.contains(path))
    return &m_effects[path].first;
  return &(m_effects[path] = { ShaderManager::makeEffect(path, layout), layout }).first;
}

}
