#include "object.h"

#include "display/graphical_resource.h"
#include "physics/physxlib.h"
#include "utils/debug.h"
#include "display/renderer.h"

namespace pbl
{

std::shared_ptr<GenericBuffer> WorldObject::s_objectConstantBuffer;

std::unique_ptr<WorldProp> WorldProp::makeObjectFromFile(GraphicalResourceRegistry &resources, const std::wstring &meshFilePath)
{
  return std::make_unique<WorldProp>(resources.loadMesh(meshFilePath));
}

std::unique_ptr<WorldProp> WorldProp::makePhysicsfullObjectFromFile(
  GraphicalResourceRegistry &resources, const std::wstring &meshFilePath, const std::wstring &physicsMeshFilePath, const std::wstring &effectFilePath)
{
  std::shared_ptr<Model> model;
  std::unique_ptr<WorldProp> prop;
  if(meshFilePath == physicsMeshFilePath) {
    auto [loadedModel, mesh] = resources.loadModelMesh(meshFilePath);
    model.swap(loadedModel);
    prop = std::make_unique<WorldProp>(mesh);
  } else {
    prop = std::make_unique<WorldProp>(resources.loadMesh(meshFilePath));
    model = resources.loadModel(physicsMeshFilePath);
  }
  if (!effectFilePath.empty()) {
    Effect *effect = resources.loadEffect(effectFilePath, BaseVertex::getShaderVertexLayout());
    std::ranges::for_each(prop->m_mesh->getSubmeshes(), [&](auto &sm) { sm.effect = effect; });
  }
  prop->m_body = pbx::PhysicsBody(prop.get());
  physx::PxTriangleMeshGeometry meshGeometry{ makePhysicsMeshFromModel(*model) };
  physx::PxTransform transform{ {0,0,0} };
  physx::PxMaterial *material = pbx::Physics::getSdk().createMaterial(1, 1, .2f);
  prop->m_body.addActor(PxCreateStatic(pbx::Physics::getSdk(), transform, meshGeometry, *material));
  return prop;
}

physx::PxTriangleMesh *WorldProp::makePhysicsMeshFromModel(const Model &model)
{
  using namespace physx;

  std::vector<PxU32> physicsMeshIndices; physicsMeshIndices.reserve(model.vertices.size());

  std::vector<PxVec3> vertices;
  std::ranges::transform(model.vertices, std::back_inserter(vertices), [](auto &v) { return pbx::scene2physicsPosition(v.position); });

  for(size_t i = 0; i < model.indices.size(); i += 3) {
    // winding is flipped because physx uses zyx instead of xyz
    physicsMeshIndices.push_back(model.indices[i+0]);
    physicsMeshIndices.push_back(model.indices[i+2]);
    physicsMeshIndices.push_back(model.indices[i+1]);
  }

  // the mesh needs "dupplicate" vertices because normals are not smooth
  // but the physx mesh does not care about the mesh's normals but still
  // gets all that dupplicated data, there is much better to be done here
  // FUTURE create track "createPhysicsVertices/Indices" methods
  PxTriangleMeshDesc meshDesc;
  meshDesc.points.count     = static_cast<PxU32>(model.vertices.size());
  meshDesc.points.stride    = sizeof(PxVec3);
  meshDesc.points.data      = vertices.data();
  meshDesc.triangles.count  = static_cast<PxU32>(physicsMeshIndices.size() / 3);
  meshDesc.triangles.stride = 3*sizeof(PxU32);
  meshDesc.triangles.data   = physicsMeshIndices.data();
  static_assert(sizeof(pbl::Mesh::index_t) == sizeof(PxU32));

  PxCookingParams params(PxTolerancesScale(1.f));
  auto mesh = PxCreateTriangleMesh(params, meshDesc);
  PBL_ASSERT(mesh != nullptr, "Could not create a triangle mesh");
  return mesh;
}

void WorldProp::render(RenderContext &context)
{
  //if (!context.cameraFrustum.isOnFrustum(m_mesh->getBoundingBox().getRotationIndependantBoundingBox(m_transform)))
  //  return; >>> restore frustrum culling, currently it works for most props but not tracks ? (terrain culling is still done and works)
  // renderer::renderAABB(m_mesh->getBoundingBox().getRotationIndependantBoundingBox(m_transform));
  ObjectConstantData data{};
  data.matWorld = XMMatrixTranspose(m_transform.getWorldMatrix());
  s_objectConstantBuffer->setData(data);
  context.constantBufferBindings.push_back({ "cbObject", s_objectConstantBuffer.get() });
  m_mesh->draw(context);
  context.constantBufferBindings.pop_back();
}

void WorldProp::renderShadows(RenderContext &context)
{
  //if (!context.cameraFrustum.isOnFrustum(m_mesh->getBoundingBox().getRotationIndependantBoundingBox(m_transform)))
  //  return;
  ObjectConstantData data{};
  data.matWorld = XMMatrixTranspose(m_transform.getWorldMatrix());
  s_objectConstantBuffer->setData(data);
  context.constantBufferBindings.push_back({ "cbObject", s_objectConstantBuffer.get() });
  m_mesh->drawSubmeshes(context, Renderable::getShadowPassEffect(), 0, m_mesh->getSubmeshes().size());
  context.constantBufferBindings.pop_back();
}

pbx::PhysicsBody *WorldProp::buildPhysicsObject()
{
  if (m_body.getType() == pbx::PhysicsBody::EMPTY)
    return nullptr;
  m_body.getActors()[0]->setGlobalPose(pbx::scene2physicsTransform(m_transform));
  return &m_body;
}

void WorldObject::loadGlobalResources()
{
  s_objectConstantBuffer = GenericBuffer::make_buffer<ObjectConstantData>(GenericBuffer::BUFFER_CONSTANT);
}

void WorldObject::unloadGlobalResources()
{
  s_objectConstantBuffer.reset();
}

}
