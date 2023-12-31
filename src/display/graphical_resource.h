#pragma once

#include "texture.h"
#include "shader.h"
#include "mesh.h"

namespace pbl
{

/*
 * The primary way to load and store graphical resources, see the comment in graphical_managed_resource
 */
class GraphicalResourceRegistry
{
private:
  using filepath = std::wstring;
  template<class T, class K>
  using map = std::unordered_map<T, K>;
  template<class T>
  using shared_ptr = std::shared_ptr<T>;

public:
  GraphicalResourceRegistry() = default;
  ~GraphicalResourceRegistry();

  GraphicalResourceRegistry(const GraphicalResourceRegistry &) = delete;
  GraphicalResourceRegistry &operator=(const GraphicalResourceRegistry &) = delete;
  GraphicalResourceRegistry(GraphicalResourceRegistry &&) = delete;
  GraphicalResourceRegistry &operator=(GraphicalResourceRegistry &&) = delete;

  Texture loadTexture(const filepath &path);
  Cubemap loadCubemap(const filepath &path);
  Effect *loadEffect(const filepath &path, const ShaderVertexLayout &layout);
  shared_ptr<Mesh> loadMesh(const filepath &path);
  shared_ptr<Model> loadModel(const filepath &path);
  // whenever possible, prefer loading both the mesh and the model at the same time
  std::pair<shared_ptr<Model>, shared_ptr<Mesh>> loadModelMesh(const filepath &path);

  void reloadShaders();

private:
  map<filepath, Texture> m_texturesCache;
  map<filepath, Cubemap> m_cubemapsCache;
  map<filepath, std::pair<Effect, ShaderVertexLayout>>  m_effects;
  map<filepath, shared_ptr<Mesh>>  m_meshes;
  map<filepath, shared_ptr<Model>> m_models;
};

}