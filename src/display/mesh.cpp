#include "mesh.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

#include "graphical_resource.h"
#include "engine/windowsengine.h"
#include "directxlib.h"
#include "utils/debug.h"

using namespace DirectX;
namespace fs = std::filesystem;

namespace pbl
{

std::shared_ptr<GenericBuffer> s_meshConstantBuffer;

ShaderVertexLayout BaseVertex::getShaderVertexLayout()
{
  return ShaderVertexLayout{}
    .addField<float>("POSITION", 3)
    .addField<float>("NORMAL", 3)
    .addField<float>("TEXCOORD", 2);
}

void Mesh::draw(RenderContext &context) const
{
  auto &d3context = WindowsEngine::d3dcontext();
  const UINT stride = m_vertexSize;
  const UINT offset = 0;
  d3context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  d3context.IASetVertexBuffers(0, 1, &m_vbo.getRawBuffer(), &stride, &offset);
  d3context.IASetIndexBuffer(m_ibo.getRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
  static_assert(sizeof(index_t)*8 == 32);

  for (const SubMesh &submesh : m_submeshes) {
    s_meshConstantBuffer->setData(submesh.material);
    submesh.effect->bindBuffer(*s_meshConstantBuffer, "cbMaterial");
    context.bindTo(*submesh.effect);
    for (const SamplerBinding &binding : submesh.samplers)
      submesh.effect->bindSampler(binding.sampler, binding.bindingName);
    for (const TextureBinding &binding : submesh.textures) {
      if(binding.texture.getRawTexture() != nullptr)
        submesh.effect->bindTexture(binding.texture, binding.bindingName);
    }
    submesh.effect->bind();

    d3context.DrawIndexed(submesh.indexCount, submesh.indexOffset, 0);
  }
}

void Mesh::drawSimilarSubmeshes(RenderContext &context, size_t submeshBegin, size_t submeshEnd) const
{
  auto &d3context = WindowsEngine::d3dcontext();
  const UINT stride = m_vertexSize;
  const UINT offset = 0;
  d3context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  d3context.IASetVertexBuffers(0, 1, &m_vbo.getRawBuffer(), &stride, &offset);
  d3context.IASetIndexBuffer(m_ibo.getRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
  static_assert(sizeof(index_t)*8 == 32);

  const SubMesh &firstSubmesh = m_submeshes[submeshBegin];
  const SubMesh &lastSubmesh = m_submeshes[submeshEnd-1];

  s_meshConstantBuffer->setData(firstSubmesh.material);
  firstSubmesh.effect->bindBuffer(*s_meshConstantBuffer, "cbMaterial");
  context.bindTo(*firstSubmesh.effect);
  for (const SamplerBinding &binding : firstSubmesh.samplers)
    firstSubmesh.effect->bindSampler(binding.sampler, binding.bindingName);
  for (const TextureBinding &binding : firstSubmesh.textures) {
    if(binding.texture.getRawTexture() != nullptr)
      firstSubmesh.effect->bindTexture(binding.texture, binding.bindingName);
  }
  firstSubmesh.effect->bind();

  index_t indexCount = lastSubmesh.indexCount + lastSubmesh.indexOffset - firstSubmesh.indexOffset;
  d3context.DrawIndexed(indexCount, firstSubmesh.indexOffset, 0);
}

void Mesh::drawSubmeshes(RenderContext &context, Effect *effect, size_t submeshBegin, size_t submeshEnd) const
{
  auto &d3context = WindowsEngine::d3dcontext();
  const UINT stride = m_vertexSize;
  const UINT offset = 0;
  d3context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  d3context.IASetVertexBuffers(0, 1, &m_vbo.getRawBuffer(), &stride, &offset);
  d3context.IASetIndexBuffer(m_ibo.getRawBuffer(), DXGI_FORMAT_R32_UINT, 0);

  const SubMesh &firstSubmesh = m_submeshes[submeshBegin];
  const SubMesh &lastSubmesh = m_submeshes[submeshEnd-1];

  context.bindTo(*effect);
  effect->bind();
  index_t indexCount = lastSubmesh.indexCount + lastSubmesh.indexOffset - firstSubmesh.indexOffset;
  d3context.DrawIndexed(indexCount, firstSubmesh.indexOffset, 0);
}

AABB Mesh::computeBoundingBox(const std::vector<BaseVertex> &vertices)
{
  float h = std::numeric_limits<float>::max(), l = std::numeric_limits<float>::lowest();
  rvec3 minP{ h,h,h }, maxP{ l,l,l };
  for(const BaseVertex &v : vertices) {
    minP.x = std::min(v.position.x, minP.x);
    minP.y = std::min(v.position.y, minP.y);
    minP.z = std::min(v.position.z, minP.z);
    maxP.x = std::max(v.position.x, maxP.x);
    maxP.y = std::max(v.position.y, maxP.y);
    maxP.z = std::max(v.position.z, maxP.z);
  }
  return AABB(XMLoadFloat3(&minP), XMLoadFloat3(&maxP)-XMLoadFloat3(&minP));
}

void Mesh::loadGlobalResources()
{
  s_meshConstantBuffer = GenericBuffer::make_buffer<Material>(GenericBuffer::BUFFER_CONSTANT);
}

void Mesh::unloadGlobalResources()
{
  s_meshConstantBuffer.reset();
}

namespace objloader
{

using index_t = Mesh::index_t;
using vertexcachekey_t = std::tuple<int, int, int, size_t>; // pos/norm/uv/mat

struct MaterialData
{
  std::string name;
  rvec3 ambiant{ 0,0,0 };
  rvec3 diffuse{ 0,0,0 };
  rvec3 specular{ 0,0,0 };
  float specularExponent = 0;
  float dissolve = 0; // 1-transparency
  float opticalDensity = 0; // index of refraction
  Texture ambiantTexture;
  // FUTURE add all other kinds of textures
};

struct VertexData
{
  rvec3 position;
  rvec3 normal;
  rvec2 texCoord;
  size_t materialIndex;
};

static const MaterialData DEFAULT_MATERIAL;

static void skipStreamText(std::istream &stream, const char *text)
{
  char c;
  while (*text) {
    if ((c = static_cast<char>(stream.get())) != *text) {
      std::cerr << "Expected " << *text << " but got " << c;
      throw std::runtime_error("Unexpected charactor in stream");
    }
    text++;
  }
}

void loadMaterialFile(const fs::path &filePath, std::vector<MaterialData> &materials, GraphicalResourceRegistry &resources)
{
  std::ifstream matFile{ filePath };
  if (!matFile) throw std::runtime_error("Could not load material file");

  size_t currentMaterial = materials.size();

  int currentLineIndex = 0;
  for (std::string currentLine; matFile; currentLineIndex++) {
    std::getline(matFile, currentLine);
    size_t space = currentLine.find(' ');
    if (currentLine.starts_with('#') || space == std::string::npos)
      continue;

    std::stringstream ss(currentLine.substr(space + 1));
    float f1, f2, f3;
    if (currentLine.starts_with("Ka ")) { // ambiant
      ss >> f1 >> f2 >> f3;
      materials[currentMaterial].ambiant = { f1, f2, f3 };

    } else if (currentLine.starts_with("Ks ")) { // specular
      ss >> f1 >> f2 >> f3;
      materials[currentMaterial].specular = { f1, f2, f3 };

    } else if (currentLine.starts_with("Kd ")) { // diffuse
      ss >> f1 >> f2 >> f3;
      materials[currentMaterial].diffuse = { f1, f2, f3 };

    } else if (currentLine.starts_with("d ")) { // dissolve
      ss >> materials[currentMaterial].dissolve;

    } else if (currentLine.starts_with("Ni ")) { // index of refraction
      ss >> materials[currentMaterial].opticalDensity;

    } else if (currentLine.starts_with("Ns ")) { // specular exponent
      ss >> materials[currentMaterial].specularExponent;

    } else if (currentLine.starts_with("map_Kd ")) { // ambiant texture
      std::string ambiantTexturePath;
      std::getline(ss, ambiantTexturePath);
      if (ambiantTexturePath.ends_with(".png"))
        ambiantTexturePath.replace(ambiantTexturePath.end()-4, ambiantTexturePath.end(), ".dds");
      materials[currentMaterial].ambiantTexture = resources.loadTexture(filePath.parent_path() / ambiantTexturePath);

    } else if (currentLine.starts_with("newmtl ")) { // switch to a new material
      MaterialData &mat = materials.emplace_back();
      std::getline(ss, mat.name);
      currentMaterial = materials.size()-1;

    } else { // unrecognized line
      continue;

    }
  }

  if (!matFile && !matFile.eof())
    throw std::runtime_error("Could not fully read a material file, error on line " + std::to_string(currentLineIndex));
}

Mesh buildMeshFromRawData(
  const std::vector<VertexData> &verticesData,
  const std::vector<BaseVertex> &vertices,
  const std::vector<index_t> &indices,
  const std::vector<MaterialData> &materialsData,
  GraphicalResourceRegistry &resources)
{
  std::vector<Mesh::SubMesh> submeshes;

  PBL_ASSERT(vertices.size() * sizeof(BaseVertex) <= (std::numeric_limits<UINT>::max)(), "Too many indices for vbo");
  PBL_ASSERT(indices.size() <= (std::numeric_limits<UINT>::max)(), "Too many indices for ibo");

  ShaderVertexLayout layout = BaseVertex::getShaderVertexLayout();

  // build submeshes
  for (size_t firstGroupVertex = 0, i = 0; i < indices.size(); i++) {
    if (i != indices.size() - 1 && verticesData[indices[i+1]].materialIndex == verticesData[indices[i]].materialIndex)
      continue;
    const MaterialData &materialData = materialsData.empty() ? DEFAULT_MATERIAL : materialsData[verticesData[indices[i]].materialIndex];
    Mesh::SubMesh &submesh = submeshes.emplace_back();
    submesh.indexCount = static_cast<index_t>(i - firstGroupVertex + 1);
    submesh.indexOffset = static_cast<index_t>(firstGroupVertex);
    submesh.textures = std::vector<TextureBinding>{ { "objectTexture", materialData.ambiantTexture } };
    submesh.samplers = std::vector<SamplerBinding>{ { "samplerState", TextureManager::getSampler(SamplerState::BASIC) } };
    submesh.material.diffuse  = { materialData.diffuse.x,  materialData.diffuse.y,  materialData.diffuse.z,  1 };
    submesh.material.specular = { materialData.specular.x, materialData.specular.y, materialData.specular.z, 1 };
    submesh.material.specularExponent = materialData.dissolve;
    submesh.effect = resources.loadEffect(L"res/shaders/miniphong.fx", layout);
    firstGroupVertex = i+1;
  }

  GenericBuffer vbo(sizeof(BaseVertex) * vertices.size(), GenericBuffer::BUFFER_VERTEX, vertices.data());
  GenericBuffer ibo(sizeof(index_t) * indices.size(), GenericBuffer::BUFFER_INDEX, indices.data());

  return Mesh(
    std::move(ibo),
    std::move(vbo),
    sizeof(BaseVertex),
    std::move(submeshes),
    Mesh::computeBoundingBox(vertices)
  );
}

std::pair<Model, Mesh> loadMeshFile(const fs::path &path, GraphicalResourceRegistry *resources)
{
  using namespace objloader;

  std::ifstream modelFile{ path };
  if (!modelFile) throw std::runtime_error("Could not open model file");

  std::vector<rvec3> positions({ { 0,0,0 } });
  std::vector<rvec3> normals({ { 0,0,0 } });
  std::vector<rvec2> uvs({ { 0,0 } });

  std::vector<vertexcachekey_t> cachedVertices;
  size_t currentMaterial = 0;

  std::vector<MaterialData> materials;
  std::vector<index_t> indices;
  std::vector<VertexData> vertices;

  int currentLineIndex = 0;
  for (std::string currentLine; modelFile; currentLineIndex++) {
    std::getline(modelFile, currentLine);
    size_t space = currentLine.find(' ');
    if (currentLine.starts_with('#') || space == std::string::npos)
      continue;

    std::stringstream ss(currentLine.substr(space + 1));
    float f1, f2, f3;
    int i1, i2, i3;
    if (currentLine.starts_with("v ")) { // vertex
      ss >> f1 >> f2 >> f3;
      positions.emplace_back(f1, f2, f3);

    } else if (currentLine.starts_with("vt ")) { // texture coordinate
      ss >> f1 >> f2;
      uvs.emplace_back(f1, f2);

    } else if (currentLine.starts_with("vn ")) { // normal
      ss >> f1 >> f2 >> f3;
      normals.emplace_back(f1, f2, f3);

    } else if (currentLine.starts_with("f ")) { // face
      index_t surfaceIndices[4] { 0,0,0,0 };
      bool isQuadSurface = false;
      for (size_t i = 0; i < std::size(surfaceIndices); i++) {
        // read vertex
        ss >> i1;
        if(!ss) continue;
        if (i == 3) isQuadSurface = true;
        skipStreamText(ss, "/");
        // read optional uv
        if (ss.peek() != '/') ss >> i2; else i2 = 0;
        skipStreamText(ss, "/");
        // read normal
        ss >> i3;

        vertexcachekey_t cacheKey{ i1, i2, i3, currentMaterial };
        auto inCacheIndex = std::ranges::find(cachedVertices, cacheKey);
        if (inCacheIndex == cachedVertices.end()) {
          vertices.push_back(VertexData{ positions[i1], normals[i3], uvs[i2], currentMaterial });
          surfaceIndices[i] = static_cast<index_t>(cachedVertices.size());
          cachedVertices.push_back(cacheKey);
        } else {
          surfaceIndices[i] = static_cast<index_t>(inCacheIndex - cachedVertices.begin());
        }
      }

      indices.push_back(surfaceIndices[0]);
      indices.push_back(surfaceIndices[1]);
      indices.push_back(surfaceIndices[2]);
      if (isQuadSurface) {
        indices.push_back(surfaceIndices[2]);
        indices.push_back(surfaceIndices[3]);
        indices.push_back(surfaceIndices[0]);
      }

    } else if (currentLine.starts_with("mtllib ") && resources != nullptr) { // load a new material file
      std::string materialFilePath;
      std::getline(ss, materialFilePath);
      loadMaterialFile(path.parent_path() / materialFilePath, materials, *resources);

    } else if (currentLine.starts_with("usemtl ") && resources != nullptr)  { // use a material for the next object
      std::string matName;
      std::getline(ss, matName);
      auto e = std::ranges::find_if(materials, [&matName](const MaterialData &mat) { return mat.name == matName; });
      if (e == materials.end()) throw std::runtime_error("Undeclared material used");
      currentMaterial = e - materials.begin();

    } else { // unrecognized line
      continue;

    }
  }

  if (!modelFile && !modelFile.eof())
    throw std::runtime_error("Could not fully read a model file, error on line " + std::to_string(currentLineIndex+1));

  std::pair<Model, Mesh> meshModel;
  auto &[model, mesh] = meshModel;

  model.indices = std::move(indices);
  model.vertices.reserve(vertices.size());
  std::ranges::transform(vertices, std::back_inserter(model.vertices), [](const VertexData &v) {
    return BaseVertex{ v.position, v.normal, v.texCoord };
  });

  if(resources != nullptr)
    mesh = buildMeshFromRawData(vertices, model.vertices, model.indices, materials, *resources);

  return meshModel;
}

} // !namespace objloader

std::pair<Model, Mesh> ModelLoader::loadModelMesh(const char *path, GraphicalResourceRegistry *resources)
{
  return objloader::loadMeshFile(path, resources);
}

} // !namespace pbl