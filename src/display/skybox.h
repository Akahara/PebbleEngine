#pragma once

#include "texture.h"
#include "generic_buffer.h"
#include "camera.h"
#include "graphical_managed_resource.h"

namespace pbl
{

class Skybox
{
public:
  explicit Skybox(Cubemap cubemap);

  void render(const Camera &camera);

  static void loadGlobalResources(GraphicalResourceRegistry &resources);
  static void unloadGlobalResources();

private:
  Cubemap       m_cubemap;
  GenericBuffer m_constantBuffer;
};

}