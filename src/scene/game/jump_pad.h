#pragma once

#include "world/object.h"
#include "display/renderable.h"
#include "display/render_profiles.h"

namespace pbl
{

class JumpPad : public pbl::WorldProp
{
public:
	explicit JumpPad(Transform transform, pbl::GraphicalResourceRegistry &resources);

	void render(RenderContext &context) override {};
	void renderTransparent(RenderContext &context) override;

};

};