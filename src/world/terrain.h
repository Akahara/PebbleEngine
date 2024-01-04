#pragma once

#include <vector>

#include "utils/math.h"
#include "utils/aabb.h"
#include "object.h"
#include "display/mesh.h"

namespace pbx
{
class PhysicsBody;
}

namespace pbl
{

class GraphicalResourceRegistry;

struct TerrainSettings {
  float worldWidth, worldHeight;
  float worldZScale;
  float uvScale;
};

class Terrain : public WorldObject
{
public:
  struct Chunk {
    AABB boundingBox;
  };

public:
  Terrain(const char *filename, GraphicalResourceRegistry &resources, TerrainSettings settings);

  vec3  sampleNormalAt(int gx, int gy) const;
  float getHeightAt(float x, float y) const;
  float getHeightAtUnsafe(float x, float y) const;
  float getHeightAtGrid(int gx, int gy) const;
  const std::vector<Chunk> &getChunks() const { return m_chunks; }

  void updateTransform(); // must be called after a transform update
  pbx::PhysicsBody *buildPhysicsObject() override;

  void render(RenderContext &context) override;

private:
  void rebuildChunks();

  std::vector<BaseVertex> buildVertices(float uvscale) const;
  static std::vector<Mesh::index_t> buildIndices(size_t gridWidth, size_t gridHeight);
  static std::vector<Mesh::SubMesh> buildChunks(GraphicalResourceRegistry &resources, size_t gridWidth, size_t gridHeight);

private:
  size_t             m_gridWidth, m_gridHeight;
  float              m_worldWidth, m_worldHeight, m_worldZScale; // before transform application
  std::vector<float> m_heights; // contains linearized heights in range 0..1
  std::vector<Chunk> m_chunks;
  pbx::PhysicsBody   m_physicsBody;
  Mesh               m_mesh;
};

}
