#pragma once

#include <functional>

#include "object.h"
#include "display/mesh.h"

namespace pbl
{
class TriggerBox : public WorldObject
{
public:
	explicit TriggerBox(const Transform &transform);
	TriggerBox(const std::function<void()> &callback, const Transform &transform);
	TriggerBox(const std::function<void(TriggerBox &self)> &callback, const Transform &transform);

	void setCallback(const std::function<void()> &callback);
	void setCallback(const std::function<void(TriggerBox &self)> &callback);
	void trigger() { if(m_onTriggerCallback) m_onTriggerCallback(*this); }

	pbx::PhysicsBody *buildPhysicsObject() override;

	void render(RenderContext &context) override {}

private:
  std::function<void(TriggerBox &self)> m_onTriggerCallback;
  pbx::PhysicsBody m_physicsBody;
};

}