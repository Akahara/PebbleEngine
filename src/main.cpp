#include <tchar.h>
#include <Windows.h>

#include "engine/windowsengine.h"
#include "utils/debug.h"

#if 1 // set to 0 when building without the windows subsystem, this allows you to have an external console window
int APIENTRY _tWinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPTSTR    lpCmdLine,
  int       nCmdShow)
{
#else
int main()
{
  HINSTANCE hInstance = GetModuleHandle(nullptr);
#endif

  srand(static_cast<unsigned int>(time(nullptr))); // rand() is only used in non-critical parts of the program where initializing a random engine is overdoing it

#ifndef PBL_ISDEBUG
  try
  {
#endif
		pbl::EngineSettings settings;
		settings.appTitle = "Celespeed";
	  pbl::WindowsEngine engine{ hInstance, std::move(settings) };
	  engine.run();

	  return 0;
#ifndef PBL_ISDEBUG
  }
  catch (const std::exception& e)
  {
	  wchar_t message[512];
	  mbstowcs_s(nullptr, message, e.what(), std::size(message) - 1);
	  MessageBox(nullptr, message, L"Fatal error!", MB_ICONWARNING);
	  return -1;
  }
#endif
}