#pragma once

#include <memory>

#include "device.h"

// forward declaration to avoid exposing both the winapi and directx
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Texture2D;
#ifndef _WINDEF_
struct HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;
struct HWND__;
typedef HWND__* HWND;
#endif


namespace pbl
{

class FrameBuffer;

class D3DDevice : public Device {
public:
  D3DDevice(DeviceMode mode, HWND hWnd);
  virtual ~D3DDevice() override;

private:
  void resizeWindow(int newWidth, int newHeight) override;
  virtual void present() override;

  void initSwapChain(DeviceMode mode, const HWND hWnd);
  void initRenderTargets();

public:
  ID3D11Device &getDevice() const { return *m_device; }
  ID3D11DeviceContext &getImmediateContext() const { return *m_immediateContext; }
  IDXGISwapChain &getSwapChain() const { return *m_swapChain; }
  FrameBuffer &getBackbuffer() const override { return *m_backbuffer; }

private:
  ID3D11Device            *m_device = nullptr;
  ID3D11DeviceContext     *m_immediateContext = nullptr;
  IDXGISwapChain          *m_swapChain = nullptr;
  std::unique_ptr<FrameBuffer> m_backbuffer;
};

}