#pragma once

#include <functional>
#include <memory>
#include <stdint.h>
#include <vector>

#include "device.h"

//#define PBL_NOTITLEBAR

namespace pbl
{
class FrameBuffer;

enum class DeviceMode {
  WINDOWED,
  FULLSCREEN,
};

using ScreenResizeEventHandler = std::function<void(int, int)>;

class Device {
public:
  virtual ~Device() = default;
  virtual void present() = 0;
  virtual FrameBuffer &getBackbuffer() const = 0;

  void performWindowResize(int newWidth, int newHeight);

  static void addWindowResizeEventHandler(std::weak_ptr<ScreenResizeEventHandler> resizeHandler);
  static uint32_t getWinWidth() { return s_winWidth; }
  static uint32_t getWinHeight() { return s_winHeight; }

private:
  virtual void resizeWindow(int newWidth, int newHeight) = 0;

protected:
  static std::vector<std::weak_ptr<ScreenResizeEventHandler>> s_screenResizeEventHandler;
  static uint32_t s_winWidth;
  static uint32_t s_winHeight;
};

}
