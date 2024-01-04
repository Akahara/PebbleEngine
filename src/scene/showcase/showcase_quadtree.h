#pragma once

#include <random>

#include "showcases.h"
#include "world/object.h"

// HUGE hijack, exists only to be able to access individual quad tree cells
#define private public
#include "world/quad_tree.h"
#undef private

class Cube : public WorldProp
{
public:
  Cube(GraphicalResourceRegistry &resources, float x, float y, float w, float h, float r)
    : WorldProp(resources.loadMesh(L"res/models/2_cube.obj"))
  {
    m_transform.position = { x, r, y, 1 };
    m_transform.scale = { w, .2f, h, 0 };
    m_aabb = AABB(m_transform.position - m_transform.scale*.5f, m_transform.scale);
  }

  AABB getAABB() const { return m_aabb; }

private:
  AABB m_aabb;
};

struct CubeRegionMapper
{
  QuadTreeRegion operator()(const Cube *cube) const
  {
    rvec3 p, hs;
    XMStoreFloat3(&p, cube->getTransform().position);
    XMStoreFloat3(&hs, cube->getTransform().scale/2.f);
    return QuadTreeRegion{ p.x - hs.x, p.z - hs.z, p.x + hs.x, p.z + hs.z };
  }
};

/*
 * The structure that allows to search in a quad-tree for objects that
 * are visible to a camera's frustum.
 *
 * Note that, as quad-trees operate on the 2D plane, some objects will
 * be reported "visible" although they should not be. The point is that
 * all visible objects are successfully reported.
 * If it is critical that objects that are not visible are discarded,
 * simply do a frustum-intersection check afterwise.
 */
struct FrustumSearch {
  static constexpr float WORLD_MIN_Y = -1e6;
  static constexpr float WORLD_MAX_Y = 1e6;

  Frustum frustum;

  std::pair<bool, bool> operator()(const QuadTreeRegion &region) const
  {
    bool contains = false; // not quite sure of how to do the math here, in any case checking for overlaps is always enough (less performant though)
    bool overlaps = frustum.isOnFrustum(AABB{ { region.minX, WORLD_MIN_Y, region.minY }, { region.maxX-region.minX, WORLD_MAX_Y-WORLD_MIN_Y, region.maxY-region.minY } });
    return { contains, overlaps };
  }
};

class ShowcaseQuadTree : public ShowcaseScene
{
public:
  using QTree = QuadTree<Cube *, CubeRegionMapper>;

  ShowcaseQuadTree()
  {
    addObjects(100);
    m_searchCamera.setProjection(PerspectiveProjection{});
  }

  float rand(float min = 0, float max=1)
  {
    return std::uniform_real_distribution(min, max)(m_gen32);
  }

protected:
  void render(const Camera& camera) override
  {
    ShowcaseScene::render(camera);

    logs::scene.beginWindow();
    ImGui::Separator();

    // in a real use-case the reset would probably be done every 6-7 frames
    // and there would likely be two, one for static objects that never gets
    // reset and one for dynamic objects
    PBL_IMGUI_PROFILE("Reset duration",
    m_quadTree.clear();
    for (auto &cube : m_objects)
      m_quadTree.add(std::static_pointer_cast<Cube>(cube).get());
    )

    std::vector<Cube *> cubesInRegion;
    PBL_IMGUI_PROFILE("QuadTree fetch",
    if(m_useSearchRegion) {
      m_quadTree.collectInBounds(cubesInRegion, m_searchRegion);
    } else {
      m_quadTree.collectMapInShape<Cube *>(cubesInRegion, FrustumSearch{ Frustum::createFrustumFromCamera(m_searchCamera) });
    }
    )
    for (auto &obj : cubesInRegion) {
        Transform aabbT{};
        aabbT.position = obj->getAABB().getOrigin()+obj->getAABB().getSize()*.5f;
        aabbT.scale = obj->getAABB().getSize()*1.05f;
      renderer::renderRect(aabbT.getWorldMatrix(), 0xff00ffff);
    }

    if(m_useSearchRegion) {
      renderer::renderLine(rvec3{ m_searchRegion.minX, +.5f, m_searchRegion.minY }, rvec3{ m_searchRegion.maxX, +.5f, m_searchRegion.minY }, 0xff305dec);
      renderer::renderLine(rvec3{ m_searchRegion.minX, +.5f, m_searchRegion.maxY }, rvec3{ m_searchRegion.maxX, +.5f, m_searchRegion.maxY }, 0xff305dec);
      renderer::renderLine(rvec3{ m_searchRegion.minX, +.5f, m_searchRegion.minY }, rvec3{ m_searchRegion.minX, +.5f, m_searchRegion.maxY }, 0xff305dec);
      renderer::renderLine(rvec3{ m_searchRegion.maxX, +.5f, m_searchRegion.minY }, rvec3{ m_searchRegion.maxX, +.5f, m_searchRegion.maxY }, 0xff305dec);
    } else {
      renderer::renderCameraOutline(m_searchCamera);
    }

    ImGui::Checkbox("Use search region", &m_useSearchRegion);
    ImGui::BeginDisabled(m_useSearchRegion);
    if (ImGui::DragFloat3("position", (float *)&m_searchCamera.getPosition(), .4f))
      m_searchCamera.updateViewMatrix();
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!m_useSearchRegion);
    ImGui::DragFloatRange2("Region X", &m_searchRegion.minX, &m_searchRegion.maxX, .5f, m_quadTree.getSpanningRegion().minX, m_quadTree.getSpanningRegion().maxX);
    ImGui::DragFloatRange2("Region Y", &m_searchRegion.minY, &m_searchRegion.maxY, .5f, m_quadTree.getSpanningRegion().minY, m_quadTree.getSpanningRegion().maxY);
    ImGui::EndDisabled();
    ImGui::Text("Objects: %zu/%zu", cubesInRegion.size(), m_objects.size());

    int c = 0; drawQuadTreeCell(m_quadTree.m_rootCell, c);

    if (ImGui::Button("Add objects"))
      addObjects(100);
    ImGui::SameLine();
    if (ImGui::Button("Clear objects"))
      m_objects.clear();

    renderer::sendDebugDraws(m_freecam.getCamera());
    ImGui::Separator();
    ImGui::End();
  }

private:
  void addObjects(size_t count)
  {
    for(size_t i = 0; i < count; i++)
    {
      float w = rand(.5f, 5.f);
      float h = rand(.5f, 5.f);
      float x = rand(m_quadTree.getSpanningRegion().minX + w*.5f, m_quadTree.getSpanningRegion().maxX - w*.5f);
      float y = rand(m_quadTree.getSpanningRegion().minY + h*.5f, m_quadTree.getSpanningRegion().maxY - h*.5f);
      auto cube = std::make_shared<Cube>(m_graphicalResources, x, y, w, h, rand(-1,1));
      m_objects.push_back(std::move(cube));
    }
  }

  void drawQuadTreeCell(const QTree::QuadTreeCell &tree, int& colorIndex) 
  {
    if (m_colors.size() == colorIndex) m_colors.push_back((unsigned int)rand(0, 0xffffff) | 0xff000000);
    unsigned int color = m_colors[colorIndex];
    QuadTreeRegion r = tree.getRegion();
    float dx = -.01f*(r.maxX - r.minX), dy = -.01f*(r.maxY - r.minY);
    r.minX += dx; r.maxX -= dx;
    r.minY += dy; r.maxY -= dy;
    renderer::renderLine(rvec3{ r.minX, 0, r.minY }, rvec3{ r.maxX, 0, r.minY }, color);
    renderer::renderLine(rvec3{ r.minX, 0, r.maxY }, rvec3{ r.maxX, 0, r.maxY }, color);
    renderer::renderLine(rvec3{ r.minX, 0, r.minY }, rvec3{ r.minX, 0, r.maxY }, color);
    renderer::renderLine(rvec3{ r.maxX, 0, r.minY }, rvec3{ r.maxX, 0, r.maxY }, color);
    colorIndex++;
    for (const auto &[key, value] : tree.m_members)
      renderer::renderAABB(key->getAABB(), color);
    if(tree.m_subcells) {
      drawQuadTreeCell(tree.m_subcells[0], colorIndex);
      drawQuadTreeCell(tree.m_subcells[1], colorIndex);
      drawQuadTreeCell(tree.m_subcells[2], colorIndex);
      drawQuadTreeCell(tree.m_subcells[3], colorIndex);
    }
  }

private:
  QTree m_quadTree{{ -1, -1, +101, +101 }};

  bool           m_useSearchRegion = true;
  QuadTreeRegion m_searchRegion{ 40, 40, 60, 60 };
  Camera         m_searchCamera;

  std::vector<unsigned int> m_colors;
  std::mt19937 m_gen32;
};
