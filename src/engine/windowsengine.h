#pragma once

#include "engine.h"
#include "d3ddevice.h"

// forward declaration to avoid exposing the winapi
#ifndef _WINDEF_
struct HACCEL__;
typedef HACCEL__* HACCEL;
#endif

namespace pbl
{
class Effect;

class WindowsEngine : public EngineBase
{
public:
  WindowsEngine(HINSTANCE hInstance, EngineSettings settings);
  virtual ~WindowsEngine() override;

  static D3DDevice &device() { return s_engineSingleton->getDevice(); }
  static ID3D11Device &d3ddevice() { return s_engineSingleton->getDevice().getDevice(); }
  static ID3D11DeviceContext &d3dcontext() { return s_engineSingleton->getDevice().getImmediateContext(); }
  static void exit();
private:
  D3DDevice &getDevice() { return reinterpret_cast<D3DDevice&>(EngineBase::getDevice()); }
  void initAppInstance();
  void showWindow();

protected:
  virtual std::unique_ptr<Device> createDeviceSpecific(DeviceMode mode) override;
  virtual void beginRenderSceneSpecific() override;
  virtual void endRenderSceneSpecific() override;
  virtual bool pollEvents() override;

private:
  HINSTANCE m_appInstance;
  HACCEL    m_accelTable = nullptr;
  HWND      m_mainWindowHandle = nullptr;

  static WindowsEngine *s_engineSingleton;
};

}