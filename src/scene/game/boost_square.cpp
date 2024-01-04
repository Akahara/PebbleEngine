#include "boost_square.h"

#include "display/graphical_resource.h"
#include "display/mesh.h"

using namespace pbl;

static constexpr float INSIDE_GAP = .15f;

BoostSquare::BoostSquare(Transform transform, GraphicalResourceRegistry &resources)
{
	std::vector<Mesh::index_t> indices =
	{
		//pavé extérieur
		0,8,1, 1,8,9, 1,9,3, 3,9,11,
		3,11,2, 2,11,10, 2,10,0, 0,10,8,

		0,6,2, 6,0,4,

		5,3,7, 3,5,1,

		0,1,5, 5,4,0,

		3,2,6, 6,7,3,

		5,13,4, 4,13,12, 4,12,6, 6,12,14,
		6,14,7, 7,14,15, 7,15,5, 5,15,13,


		//pavé intérieur
		8,13,9, 13,8,12,

		9,15,11, 15,9,13,

		11,14,10, 14,11,15,

		8,14,12, 14,8,10
	};
	std::vector<BaseVertex> vertices = 
	{ 
		{{-1,-1,-1}}, {{1,-1,-1}}, {{-1,1,-1}}, {{1,1,-1}},
	    {{-1,-1,1}}, {{1,-1,1}}, {{-1,1,1}}, {{1,1,1}},
		{{-(1-INSIDE_GAP),-(1-INSIDE_GAP),-(1-INSIDE_GAP)}}, {{(1-INSIDE_GAP),-(1-INSIDE_GAP),-(1-INSIDE_GAP)}}, 
		{{-(1-INSIDE_GAP),(1-INSIDE_GAP),-(1-INSIDE_GAP)}}, {{(1-INSIDE_GAP),(1-INSIDE_GAP),-(1-INSIDE_GAP)}},
		{{-(1-INSIDE_GAP),-(1-INSIDE_GAP),(1-INSIDE_GAP)}}, {{(1-INSIDE_GAP),-(1-INSIDE_GAP),(1-INSIDE_GAP)}}, 
		{{-(1-INSIDE_GAP),(1-INSIDE_GAP),(1-INSIDE_GAP)}}, {{(1-INSIDE_GAP),(1-INSIDE_GAP),(1-INSIDE_GAP)}}
	};
	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(
	  std::move(indices),
	  std::move(vertices),
	  std::vector{ { Mesh::SubMesh{
		resources.loadEffect(L"res/shaders/boost_square.fx", BaseVertex::getShaderVertexLayout()),
		Material{}, {}, {}, static_cast<Mesh::index_t>(indices.size()), 0 } }
	  }
	);
	setMesh(mesh);
	m_transform = transform;
}

void BoostSquare::renderTransparent(RenderContext &context)
{
	RenderProfiles::pushBlendProfile(BlendProfile::BLEND);
	ObjectConstantData data{};
	data.matWorld = XMMatrixTranspose(m_transform.getWorldMatrix());
	s_objectConstantBuffer->setData(data);
	context.constantBufferBindings.push_back({ "cbObject", s_objectConstantBuffer.get() });
	m_mesh->draw(context);
	context.constantBufferBindings.pop_back();
	RenderProfiles::popBlendProfile();
}