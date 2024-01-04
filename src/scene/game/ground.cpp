#include "ground.h"

#include "display/graphical_resource.h"
#include "display/mesh.h"

using namespace pbl;

static std::vector<Mesh::index_t> initIndices(int frag)
{
	std::vector<Mesh::index_t> indices{};
	for (int y = 0; y < 2*frag; y++) {
		for (int x = 0; x < 2*frag; x++) {
			// L'important ici est d'utiliser la m�me formule pour identifier
			// les sommets qu'au moment de leur cr�ation
			indices.push_back( y * (2 * frag + 1) + x);
			indices.push_back((y + 1) * (2 * frag + 1) + (x + 1));
			indices.push_back(y * (2 * frag + 1) + (x + 1));
			indices.push_back((y + 1) * (2 * frag + 1) + (x + 1));
			indices.push_back( y * (2 * frag + 1) + x);
			indices.push_back((y + 1) * (2 * frag + 1) + x);
		}
	}
	return indices;
}

static std::vector<BaseVertex> buildPlane(int frag)
{
	std::vector<BaseVertex> vertices = std::vector<BaseVertex>{};
	for (int y = 0; y < 2*frag+1; y++) {
		for (int x = 0; x < 2*frag+1; x++) {
			BaseVertex vertex{};
			vertex.position = { x/(float)frag - 1.f, 0.f, y/(float)frag - 1.f };
			vertex.normal = { 0.f,1.f,0.f };
			vertex.texCoord = { (float)x/(2*frag), (float)y/(2*frag) };
			vertices.emplace_back(vertex);
		}
	}
	return vertices;
}

Ground::Ground(GraphicalResourceRegistry &resources)
{
  enum { SIZE = 40 };
  auto indices = initIndices(SIZE);
  auto vertices = buildPlane(SIZE);
  std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(
	std::move(indices),
	std::move(vertices),
	std::vector{ { Mesh::SubMesh{
	  resources.loadEffect(L"res/shaders/ground.fx", BaseVertex::getShaderVertexLayout()),
	  Material{}, {}, {}, static_cast<Mesh::index_t>(indices.size()), 0 } }
	}
  );
  float l = 1000000.f;
  mesh->setBoundingBox(AABB{ vec3{-l,-10,-l},vec3{2*l,2*10,2*l} });
  setMesh(mesh);
}
