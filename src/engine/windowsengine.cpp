#include "windowsengine.h"

#define NOMINMAX
#include <Windows.h>

#include <utility>

#include "display/directxlib.h"
#include "display/frame_buffer.h"
#include "display/graphical_resource.h"
#include "utils/debug.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include "inputs/user_inputs.h"

#ifdef PBL_HASGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

static constexpr const wchar_t *APP_WINDOW_CLASS = L"pebble_engine_winclass";

namespace pbl
{

WindowsEngine *WindowsEngine::s_engineSingleton = nullptr;

WindowsEngine::WindowsEngine(HINSTANCE hInstance, EngineSettings settings)
  : EngineBase(std::move(settings))
  , m_appInstance(hInstance)
{
  s_engineSingleton = this;

  initAppInstance();
  UserInputs::setWindowsHandle(m_appInstance, m_mainWindowHandle);
  EngineBase::init();

#ifdef PBL_HASGUI
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  ImGui_ImplWin32_Init(m_mainWindowHandle);
  ImGui_ImplDX11_Init(&d3ddevice(), &d3dcontext());
#endif

  showWindow();
  UserInputs::loadGlobalResources();
}

WindowsEngine::~WindowsEngine()
{
  EngineBase::dispose();

#ifdef PBL_HASGUI
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
#endif

  s_engineSingleton = nullptr;
}

void WindowsEngine::exit()
{
	s_engineSingleton->EngineBase::exit();
}

std::unique_ptr<Device> pbl::WindowsEngine::createDeviceSpecific(DeviceMode mode)
{
  return std::make_unique<D3DDevice>(mode, m_mainWindowHandle);
}

void WindowsEngine::beginRenderSceneSpecific()
{
  FrameBuffer::getActiveFrameBuffer().clearTargets();

#ifdef PBL_HASGUI
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
#endif
}

void WindowsEngine::endRenderSceneSpecific()
{
#ifdef PBL_HASGUI
  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
}

bool WindowsEngine::pollEvents()
{
  MSG msg;
  bool didUserQuit = false;

  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY) {
      didUserQuit = true;
    } else if (!TranslateAccelerator(msg.hwnd, m_accelTable, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return !didUserQuit;
}

static LRESULT CALLBACK wndProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef PBL_HASGUI
  if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
  return true;
#endif

  switch (message) {
  case WM_DESTROY:
    PostQuitMessage(0);
  break;
  case WM_SETFOCUS:
    UserInputs::setNextPollRewireInputs();
    break;
  case WM_KILLFOCUS:
    UserInputs::setInputsNotAquired();
    break;
  case WM_SIZE: {
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);
    WindowsEngine::device().performWindowResize(width, height);
    break;
  }
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

void WindowsEngine::initAppInstance()
{
  WNDCLASSEX winclass{};
  winclass.cbSize = sizeof(WNDCLASSEX);
  winclass.style = CS_HREDRAW | CS_VREDRAW;
  winclass.lpfnWndProc = &wndProcCallback;
  winclass.cbClsExtra = 0;
  winclass.cbWndExtra = 0;
  winclass.hInstance = m_appInstance;
  winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
  winclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  winclass.lpszClassName = APP_WINDOW_CLASS;
  //winclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PETITMOTEUR3D));
  //winclass.hIconSm = LoadIcon(winclass.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  RegisterClassEx(&winclass);

  m_mainWindowHandle = CreateWindowW(
    APP_WINDOW_CLASS,
    utils::string2widestring(m_settings.appTitle).c_str(), 
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    0,
    1600,
    900,
    NULL,
    NULL,
    m_appInstance,
    NULL);

#ifdef PBL_NOTITLEBAR
  SetWindowLong(m_mainWindowHandle, GWL_STYLE, 0);
#endif

  if (!m_mainWindowHandle)
    throw std::runtime_error("Could not create a window handle");

  m_accelTable = LoadAccelerators(m_appInstance, APP_WINDOW_CLASS);
}

void WindowsEngine::showWindow()
{
  ShowWindow(m_mainWindowHandle, SW_SHOWNORMAL);
  UpdateWindow(m_mainWindowHandle);
}

}