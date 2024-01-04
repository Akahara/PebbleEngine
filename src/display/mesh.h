#pragma once

#include <vector>

#include "utils/math.h"
#include "texture.h"
#include "shader.h"
#include "renderable.h"
#include "graphical_managed_resource.h"

struct ID3D11Buffer;

namespace pbl
{

struct BaseVertex
{
  rvec3 position{};
  rvec3 normal{};
  rvec2 texCoord{};

  static ShaderVertexLayout getShaderVertexLayout();
};

struct Material
{
  vec3 diffuse{};
  vec3 specular{};
  float specularExponent = 2.f;
  float _padding[3];
};

struct Model
{
  using index_t = uint32_t;
  std::vector<BaseVertex> vertices;
  std::vector<index_t>    indices;
};

class Mesh
{
public:
  using index_t = Model::index_t;
  struct SubMesh
  {
    Effect                     *effect;
    Material                    material;
    std::vector<TextureBinding> textures;
    std::vector<SamplerBinding> samplers;
    index_t                     indexCount{};
    index_t                     indexOffset{};
  };

  Mesh() = default;

  Mesh(GenericBuffer &&ibo, GenericBuffer &&vbo, unsigned int vertexSize, std::vector<SubMesh> &&submeshes, AABB boundingBox)
    : m_ibo(std::move(ibo)), m_vbo(std::move(vbo)), m_vertexSize(vertexSize), m_submeshes(submeshes), m_boundingBox(boundingBox) { }
  Mesh(const std::vector<index_t> &indices, const std::vector<BaseVertex> &vertices, std::vector<SubMesh> &&submeshes)
    : Mesh(
      GenericBuffer(sizeof(index_t) * indices.size(), GenericBuffer::BUFFER_INDEX, indices.data()),
      GenericBuffer(sizeof(BaseVertex) * vertices.size(), GenericBuffer::BUFFER_VERTEX, vertices.data()),
      sizeof(BaseVertex), std::move(submeshes),
      computeBoundingBox(vertices))
  { }

  void draw(RenderContext &context) const;
  void drawSimilarSubmeshes(RenderContext &context, size_t submeshBegin, size_t submeshEnd) const;
  void drawSubmeshes(RenderContext &context, Effect *effect, size_t submeshBegin, size_t submeshEnd) const;

  std::vector<SubMesh> &getSubmeshes() { return m_submeshes; }
  const AABB &getBoundingBox() const { return m_boundingBox; }
  // used to replace bounding boxes when a mesh vertices are transformed before being displayed, use with care
  void setBoundingBox(const AABB &aabb) { m_boundingBox = aabb; }

  static AABB computeBoundingBox(const std::vector<BaseVertex> &vertices);

  static void loadGlobalResources();
  static void unloadGlobalResources();

private:
  GenericBuffer m_ibo;
  GenericBuffer m_vbo;
  unsigned int m_vertexSize{};
  std::vector<SubMesh> m_submeshes;
  AABB m_boundingBox;
};

class ModelLoader
{
public:
  static std::pair<Model, Mesh> loadModelMesh(const char *path, GraphicalResourceRegistry *resourcesLoader);

  static Model loadModel(const char *path)
  {
    return loadModelMesh(path, nullptr).first;
  }

  static Mesh loadMesh(const char *path, GraphicalResourceRegistry &resourcesLoader)
  {
    return loadModelMesh(path, &resourcesLoader).second;
  }
};

}
