#pragma once

#include <memory>
#include <string>

#include "device.h"
#include "utils/clock.h"

namespace pbl
{
class Effect;
class FrameBuffer;
class GraphicalResourceRegistry;

struct EngineSettings {
  std::string appTitle;
};

class EngineBase
{
protected:
  explicit EngineBase(EngineSettings settings);
  virtual ~EngineBase();

  void init();
  void dispose();

public:
  void run();
  void exit();

protected:
  virtual bool pollEvents() = 0;
  virtual std::unique_ptr<Device> createDeviceSpecific(DeviceMode mode) = 0;
  virtual void beginRenderSceneSpecific() = 0;
  virtual void endRenderSceneSpecific() = 0;

  Device &getDevice() { return *m_device; }

private:
  bool runFrame(double previousFrameDelta);
  bool postFrame();
  bool shouldExit = false;

 

protected:
  EngineSettings           m_settings;

  int64_t                  m_previousTime{};
  PerformanceClock         m_clock{};
  std::unique_ptr<Device>  m_device = nullptr;

  std::unique_ptr<GraphicalResourceRegistry> m_globalGraphicalResources;
  std::unique_ptr<FrameBuffer>               m_primaryFrameBuffer;
  std::shared_ptr<ScreenResizeEventHandler>  m_windowResizeEventHandler;
};

}
