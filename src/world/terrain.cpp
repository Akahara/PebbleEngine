#include "terrain.h"

#include <algorithm>

#include <stbi/stb_image.h>

#include "display/graphical_resource.h"
#include "display/renderer.h"
#include "display/texture.h"
#include "physics/physxlib.h"
#include "physics/physics.h"

namespace pbl
{

static constexpr size_t CHUNK_SIZE = 32;

Terrain::Terrain(const char *filename, GraphicalResourceRegistry &resources, TerrainSettings settings)
  : m_worldWidth(settings.worldWidth)
  , m_worldHeight(settings.worldHeight)
  , m_worldZScale(settings.worldZScale)
{
    int imageWidth, imageHeight;
    unsigned short *imageData = stbi_load_16(filename, &imageWidth, &imageHeight, nullptr, 1);
    if (imageData == nullptr) throw std::runtime_error("Could not load heightmap " + std::string(filename));
    m_gridWidth  = (imageWidth  - 1 + (CHUNK_SIZE - 1)) / CHUNK_SIZE * CHUNK_SIZE + 1;
    m_gridHeight = (imageHeight - 1 + (CHUNK_SIZE - 1)) / CHUNK_SIZE * CHUNK_SIZE + 1;

    m_heights.resize(m_gridWidth*m_gridHeight);
    float terrainMaxZ = std::numeric_limits<unsigned short>::max();
    for (int y = 0; y < m_gridHeight; ++y) {
        for (int x = 0; x < m_gridWidth; ++x) {
            unsigned short z = imageData[std::min(x, imageWidth-1) + imageWidth * std::min(y, imageHeight-1)];
            m_heights[x + y * m_gridWidth] = z / terrainMaxZ;
        }
    }
    stbi_image_free(imageData);

    std::vector<BaseVertex> vertices = buildVertices(settings.uvScale);
    std::vector<Mesh::index_t> indices = buildIndices(m_gridWidth, m_gridHeight);
    std::vector<Mesh::SubMesh> submeshes = buildChunks(resources, m_gridWidth, m_gridHeight);

    GenericBuffer vbo{ sizeof(BaseVertex)*vertices.size(), GenericBuffer::BUFFER_VERTEX, vertices.data() };
    GenericBuffer ibo{ sizeof(Mesh::index_t)*indices.size(), GenericBuffer::BUFFER_INDEX, indices.data() };
    m_mesh = Mesh(std::move(ibo), std::move(vbo), sizeof(BaseVertex), std::move(submeshes), AABB{/*never actually used*/});
    updateTransform();
}

void Terrain::render(RenderContext &context)
{
  ObjectConstantData data{};
  data.matWorld = XMMatrixTranspose(m_transform.getWorldMatrix());
  s_objectConstantBuffer->setData(data);
  context.constantBufferBindings.push_back({ "cbObject", s_objectConstantBuffer.get() });

  size_t nextFirstChunk = 0;
  for(size_t i = 0; i < m_chunks.size(); i++) {
    if (context.cameraFrustum.isOnFrustum(m_chunks[i].boundingBox))
      continue; // chunk is visible, it will be drawn in the next batch
    if (nextFirstChunk != i)
      m_mesh.drawSimilarSubmeshes(context, nextFirstChunk, i);
    nextFirstChunk = i+1;
  }
  if (nextFirstChunk != m_chunks.size())
    m_mesh.drawSimilarSubmeshes(context, nextFirstChunk, m_chunks.size());

  context.constantBufferBindings.pop_back();
}

std::vector<BaseVertex> Terrain::buildVertices(float uvScale) const
{
  std::vector<BaseVertex> vertices;

  for (size_t y = 0; y < m_gridHeight; ++y) {
    for (size_t x = 0; x < m_gridWidth; ++x) {
      float wx = x * m_worldWidth  / m_gridWidth ;
      float wz = y * m_worldHeight / m_gridHeight;
      float wy = m_heights[x + y * m_gridWidth];
      float c = .2f;
      wy = wy < c ? wy-10 : wy; // hyjack: there is not enough precision with our 16bpp heightmap texture
                                // but we still need to make it so that the bottom layer of the terrain is deep enough
      wy *= m_worldZScale;
      float uvx = x * uvScale / (m_gridWidth  - 1.f);
      float uvy = y * uvScale / (m_gridHeight - 1.f);
      BaseVertex &vertex = vertices.emplace_back();
      vertex.position = { wx, wy, wz };
      vertex.texCoord = { uvx, uvy };
      XMStoreFloat3(&vertex.normal, sampleNormalAt((int)x, (int)y));
    }
  }

  return vertices;
}

std::vector<Mesh::index_t> Terrain::buildIndices(size_t gridWidth, size_t gridHeight)
{
  std::vector<Mesh::index_t> indices;

  size_t chunkCountWidth  = (gridWidth  - 1) / CHUNK_SIZE;
  size_t chunkCountHeight = (gridHeight - 1) / CHUNK_SIZE;
  for(size_t cx = 0; cx < chunkCountWidth; cx++) {
    for(size_t cy = 0; cy < chunkCountHeight; cy++) {
      size_t chunkOriginX = cx * CHUNK_SIZE;
      size_t chunkOriginY = cy * CHUNK_SIZE;
      for (size_t y = 0; y < CHUNK_SIZE; ++y) {
        for (size_t x = 0; x < CHUNK_SIZE; ++x) {
          indices.push_back(static_cast<Mesh::index_t>((chunkOriginY + y    ) * gridWidth + (chunkOriginX + x    )));
          indices.push_back(static_cast<Mesh::index_t>((chunkOriginY + y + 1) * gridWidth + (chunkOriginX + x + 1)));
          indices.push_back(static_cast<Mesh::index_t>((chunkOriginY + y    ) * gridWidth + (chunkOriginX + x + 1)));
          indices.push_back(static_cast<Mesh::index_t>((chunkOriginY + y    ) * gridWidth + (chunkOriginX + x    )));
          indices.push_back(static_cast<Mesh::index_t>((chunkOriginY + y + 1) * gridWidth + (chunkOriginX + x    )));
          indices.push_back(static_cast<Mesh::index_t>((chunkOriginY + y + 1) * gridWidth + (chunkOriginX + x + 1)));
        }
      }
    }
  }

  return indices;
}

std::vector<Mesh::SubMesh> Terrain::buildChunks(GraphicalResourceRegistry &resources, size_t gridWidth, size_t gridHeight)
{
  using SubMesh = Mesh::SubMesh;

  std::vector<SubMesh> submeshes;

  SubMesh submesh;
  submesh.textures.push_back({ "textureLayer0", resources.loadTexture(L"res/textures/terrain.dds") });
  submesh.textures.push_back({ "textureLayer1", resources.loadTexture(L"res/textures/terrain.dds") });
  submesh.textures.push_back({ "textureLayer2", resources.loadTexture(L"res/textures/terrain.dds") });
  submesh.samplers.push_back({ "samplerState", TextureManager::getSampler(SamplerState::BASIC) });
  submesh.material.diffuse  = { 1, 1, 1, 1 };
  submesh.material.specular = { 0, 0, 0, 1 };
  submesh.material.specularExponent = 0;
  submesh.effect = resources.loadEffect(L"res/shaders/terrain_miniphong.fx", BaseVertex::getShaderVertexLayout());
  submesh.indexCount += CHUNK_SIZE*CHUNK_SIZE*6;

  for(size_t cx = 0; cx < gridWidth/CHUNK_SIZE; cx++) {
    for (size_t cy = 0; cy < gridHeight/CHUNK_SIZE; cy++) {
      submeshes.push_back(submesh);
      submesh.indexOffset += submesh.indexCount;
    }
  }

  return submeshes;
}

vec3 Terrain::sampleNormalAt(int x, int y) const
{
  using namespace DirectX;

  float cz = getHeightAtGrid(x, y);
  float dx = 2.f, dy = 2.f;
  float nxp = x == m_gridWidth  - 1 ? (--dx,cz) : getHeightAtGrid(x+1, y);
  float nyp = y == m_gridHeight - 1 ? (--dy,cz) : getHeightAtGrid(x, y+1);
  float pxp = x == 0                ? (--dx,cz) : getHeightAtGrid(x-1, y);
  float pyp = y == 0                ? (--dy,cz) : getHeightAtGrid(x, y-1);
  return XMVector3Normalize(XMVector3Cross(
    { 0.f, (nyp - pyp)*m_worldZScale, dy*m_worldHeight/m_gridHeight },
    { dx*m_worldWidth/m_gridWidth, (nxp - pxp)*m_worldZScale, 0.f }
  ));
}

float Terrain::getHeightAt(float x, float y) const
{
  x = x / m_worldWidth  * (m_gridWidth  - 1.f);
  y = y / m_worldHeight * (m_gridHeight - 1.f);
  int
    fx = std::clamp((int)x,   (int)m_gridWidth -1, 0),
    fy = std::clamp((int)y,   (int)m_gridHeight-1, 0),
    cx = std::clamp((int)x+1, (int)m_gridWidth -1, 0),
    cy = std::clamp((int)y+1, (int)m_gridHeight-1, 0);
  float lx = std::clamp(x - (float)fx, 0.f, 1.f);
  float ly = std::clamp(y - (float)fy, 0.f, 1.f);
  return
    std::lerp(
      std::lerp(getHeightAtGrid(fx, cy), getHeightAtGrid(cx, cy), lx),
      std::lerp(getHeightAtGrid(fx, cy), getHeightAtGrid(cx, cy), lx),
      ly
    );
}

float Terrain::getHeightAtUnsafe(float x, float y) const
{
  x = x / m_worldWidth  * (m_gridWidth  - 1.f);
  y = y / m_worldHeight * (m_gridHeight - 1.f);
  int
    fx = (int)x,
    fy = (int)y,
    cx = (int)x+1,
    cy = (int)y+1;
  float lx = x - (float)fx;
  float ly = y - (float)fy;
  return
    std::lerp(
      std::lerp(getHeightAtGrid(fx, cy), getHeightAtGrid(cx, cy), lx),
      std::lerp(getHeightAtGrid(fx, cy), getHeightAtGrid(cx, cy), lx),
      ly
    );
}

float Terrain::getHeightAtGrid(int gx, int gy) const
{
  return m_heights[gx + m_gridWidth * gy];
}

void Terrain::updateTransform()
{
  rebuildChunks();
}

void Terrain::rebuildChunks() {
  // update chunks
  m_chunks.resize((m_gridWidth/CHUNK_SIZE)*(m_gridHeight/CHUNK_SIZE));
  float chunkWorldWidth  = m_worldWidth  * CHUNK_SIZE / m_gridWidth  * XMVectorGetX(m_transform.scale);
  float chunkWorldHeight = m_worldHeight * CHUNK_SIZE / m_gridHeight * XMVectorGetZ(m_transform.scale);
  float chunkWorldZScale = m_worldZScale * XMVectorGetY(m_transform.scale);
  vec3 chunkSize{ chunkWorldWidth, 11*chunkWorldZScale, chunkWorldHeight };

  size_t i = 0;
  for(size_t cx = 0; cx < m_gridWidth/CHUNK_SIZE; cx++) {
    for (size_t cy = 0; cy < m_gridHeight/CHUNK_SIZE; cy++) {
      vec3 chunkOrigin = vec3{ (float)cx * chunkWorldWidth, -10*m_worldZScale, (float)cy * chunkWorldHeight } + m_transform.position;
      m_chunks[i++].boundingBox = { chunkOrigin, chunkSize };
    }
  }
}

pbx::PhysicsBody *Terrain::buildPhysicsObject()
{
  using namespace physx;
  using namespace pbx;

  constexpr PxI16 MAX_PRECISION = 10000;

  PxHeightFieldDesc desc;
  desc.format = PxHeightFieldFormat::eS16_TM;
  auto rawHeightData = std::make_unique<PxHeightFieldSample[]>(m_heights.size());
  std::ranges::transform(m_heights, rawHeightData.get(), [](float h) {
    PxHeightFieldSample sample;
    sample.height = static_cast<PxI16>(h * static_cast<float>(MAX_PRECISION));
    return sample;
  });
  desc.samples.stride = sizeof(PxHeightFieldSample);
  desc.samples.data   = rawHeightData.get();
  desc.nbColumns      = static_cast<PxU32>(m_gridWidth);
  desc.nbRows         = static_cast<PxU32>(m_gridHeight);

  PxHeightField *field = PxCreateHeightField(desc);
  PxHeightFieldGeometry fieldGeometry(field, PxMeshGeometryFlags(), m_worldZScale / static_cast<float>(MAX_PRECISION), m_worldHeight / (float)m_gridHeight, m_worldWidth / (float)m_gridWidth);
  PxMaterial *material = Physics::getSdk().createMaterial(1.f, 1.f, .5f);
  PxTransform transform( scene2physicsPosition(m_transform.position) );

  m_physicsBody = PhysicsBody(this);
  m_physicsBody.addActor(PxCreateStatic(Physics::getSdk(), transform, fieldGeometry, *material));
  return &m_physicsBody;
}

}
