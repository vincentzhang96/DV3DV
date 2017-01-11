#include "stdafx.h"
#include "main.h"
#include "OpenGLContext.h"
#include "Config.h"

INITIALIZE_EASYLOGGINGPP

OpenGLContext* oglContext = nullptr;
HWND hWnd = nullptr;
HINSTANCE hInstance = nullptr;
MSG msg;

bool active;
bool fullscreen;

void OnWindowActive()
{
	auto wasActive = active;
	active = true;
	if (active != wasActive)
	{
		LOG(DEBUG) << "Window unminimized";
		//	TODO Notify
	}
}

void OnWindowInactive()
{
	auto wasActive = active;
	active = false;
	if (active != wasActive)
	{
		LOG(DEBUG) << "Window minimized";
		//	TODO Notify
	}
}

void OnWindowResize(int newWidth, int newHeight)
{
	oglContext->ResizeWindow(newWidth, newHeight);
	LOG(TRACE) << "Window resized to " << newWidth << "x" << newHeight;

	//	TODO resetup stuff
}

void OnKeyDown(int keyCode)
{
	//	TODO
}

void OnKeyUp(int keyCode)
{
	//	TODO temporary quit mechanism
	if (keyCode == VK_ESCAPE)
	{
		PostQuitMessage(0);
	}

	//	TODO
}

///	Allocates a windows console
bool _ShowConsole()
{
	if (!AllocConsole())
	{
		LOG(ERROR) << "Failed to allocate console";
		return false;
	}
	//	Redirect STDOUT
	FILE* newStd;
	freopen_s(&newStd, "CONOUT$", "w", stdout);
	return true;
}

inline bool _DoMainLoop()
{
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		//	Handle window messages
		if (msg.message == WM_QUIT)
		{
			//	Quit requested

			//	TODO have this signal into the program instead of killing immediately
			return true;
		}
		else
		{
			//	Handle the incoming message
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	else
	{
		//	Inner loop
		oglContext->PreRender();
		//	TODO Render stuff

		oglContext->PostRender();
		//	TODO Clock this
		Sleep(10);
	}
	return false;
}

void _SetUpLogger()
{
	//	Check that the logging config file exists before attempting to load it
	auto dwAttrib = GetFileAttributesW(L"logging.conf");
	if (dwAttrib == 0xFFFFFFFF)
	{
		//	Logging configuration does not exist, so we'll use the default instead except we'll set the log file
		el::Configurations loggingConfig;
		loggingConfig.setToDefault();
		//	Set output log file
		loggingConfig.setGlobally(el::ConfigurationType::Filename, "logs/dv3dv-%datetime{%Y%M%d_%H%m%s}.log");
		//	Set all loggers to use this config
		el::Loggers::reconfigureAllLoggers(loggingConfig);
	}
	else
	{
		//	Load logging config
		el::Configurations loggingConfig("logging.conf");
		//	Set all loggers to use this config
		el::Loggers::reconfigureAllLoggers(loggingConfig);
	}

	//	Disable FATAL app aborts
	el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
}

int WINAPI WinMain(HINSTANCE hInstance,	
                   HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, 
                   int nCmdShow)
{
	_SetUpLogger();

	//	Options
	auto optShowConsole = true;
	//	TODO more

	//	Handle command line
	LPWSTR* argv;
	int argc;
	auto lpCommandLine = GetCommandLineW();
	argv = CommandLineToArgvW(lpCommandLine, &argc);
	//	Failed to parse command line, fail out
	if (argv == nullptr)
	{
		MessageBoxW(nullptr, 
		           L"Failed to parse command line.", 
		           L"DV3DV Startup Error", 
		           MB_OK | MB_ICONERROR);
		return -1;
	}
	//	Handle arguments
	//	TODO
	for (auto i = 0; i < argc; ++i)
	{
		//MessageBoxW(nullptr, argv[i], L"DV3DV", MB_OK);
	}
	//	Don't need argv anymore, free
	LocalFree(argv);

	//	Create console if necessary
	if (optShowConsole && !_ShowConsole())
	{
		if (!MessageBoxW(nullptr, 
		                L"Failed to create console window. Do you wish to continue?", 
		                L"DV3DV Startup Error", 
		                MB_YESNO | MB_ICONEXCLAMATION))
		{
			return 0;
		}
	}
	//	Go for full startup
	LOG(INFO) << "Starting...";
	//	Load config


	//	Create the window
	if (!CreateOGLWindow(L"DV3DV", 1600, 900, false))
	{
		LOG(ERROR) << "Failed to create window, stopping";
		return 0;
	}
	//	Prep for main loop
	auto exitLoop = false;
	//	Main loop
	LOG(INFO) << "Entering main loop";
	while (!exitLoop) 
	{
		exitLoop = _DoMainLoop();
	}
	//	Destroy window
	KillOGLWindow();
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ACTIVATE:
		{
			//	Minimization state
			if (!HIWORD(wParam))
			{
				OnWindowActive();
			}
			else
			{
				OnWindowInactive();
			}
			return 0;
		}
	case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
			case SC_SCREENSAVE:
			case SC_MONITORPOWER:
				//	Prevent screensaver or monitor entering powersave
				return 0;
			default:
				break;
			}
			break;
		}
	case WM_CLOSE:
		{
			//	Close requested via Window close button
			PostQuitMessage(0);
			return 0;
		}
	case WM_KEYDOWN:
		{
			OnKeyDown(wParam);
			return 0;
		}
	case WM_KEYUP:
		{
			OnKeyUp(wParam);
			return 0;
		}
	case WM_SIZE:
		{
			//	WINDOW RESIZE
			OnWindowResize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}
	default:
		break;
	}
	//	Let the default handlers handle it otherwise
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void _TrySetupFullscreen(int winWidth, int winHeight, DWORD& dwExStyle, DWORD& dwStyle)
{
	if (fullscreen)
	{
		DEVMODE devScreenSettings;
		ZeroMemory(&devScreenSettings, sizeof(devScreenSettings));
		devScreenSettings.dmSize = sizeof(devScreenSettings);
		devScreenSettings.dmPelsWidth = winWidth;
		devScreenSettings.dmPelsHeight = winHeight;
		devScreenSettings.dmBitsPerPel = 24;
		devScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		if (ChangeDisplaySettings(&devScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			LOG(INFO) << "Failed to switch to fullscreen, falling back to windowed";
			fullscreen = false;
		}
	}
	//	Set styles accordingly
	//	Note that fullscreen state may have changed when we attempted to enter fullscreen
	if (fullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW;
	}
}

bool CreateOGLWindow(LPCWSTR winTitle,
                     int winWidth, 
                     int winHeight, 
                     bool fullscreenWindow)
{
	GLuint pixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;
	RECT windowRect;
	windowRect.left = long(0);
	windowRect.right = long(winWidth);
	windowRect.top = long(0);
	windowRect.bottom = long(winHeight);
	//	Check that any previous OpenGL context was destroyed first
	if (oglContext != nullptr)
	{
		LOG(WARNING) << "Previous OpenGL context was not destroyed, possible resource leak!";
	}
	oglContext = new OpenGLContext();

	fullscreen = fullscreenWindow;

	hInstance = GetModuleHandle(nullptr);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WNDPROC(WndProc);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(nullptr, IDI_WINLOGO);	//	TODO load icon
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = DV_CLASS_NAME;
	
	if (!RegisterClass(&wc))
	{
		LOG(ERROR) << "Window registration failed";
		MessageBoxW(nullptr, 
		           L"Failed to register window.", 
		           L"DV3DV Startup Error", 
		           MB_OK | MB_ICONERROR);
		return false;
	}

	//	Attempt to enter fullscreen
	_TrySetupFullscreen(winWidth, winHeight, dwExStyle, dwStyle);
	AdjustWindowRectEx(&windowRect, dwStyle, false, dwExStyle);
	//	Create the window
	hWnd = CreateWindowExW(dwExStyle,
	                      DV_CLASS_NAME,
	                      winTitle,
	                      dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	                      0,
	                      0,
	                      windowRect.right - windowRect.left,
	                      windowRect.bottom - windowRect.top,
	                      nullptr,
	                      nullptr,
	                      hInstance,
	                      nullptr);
	if (!hWnd)
	{
		LOG(ERROR) << "Failed to create window";
		KillOGLWindow();
		MessageBoxW(nullptr, 
		           L"Failed to create window", 
		           L"DV3DV Error", 
		           MB_OK | MB_ICONERROR);
		return false;
	}
	//	Create the OpenGL context
	if (!oglContext->CreateContext(hWnd))
	{
		LOG(ERROR) << "Context creation failed";
		KillOGLWindow();
		MessageBoxW(nullptr,
			L"Failed to create context",
			L"DV3DV Error",
			MB_OK | MB_ICONERROR);
		return false;
	}
	LOG(INFO) << "OpenGL v" << oglContext->GetGLMajorVersion() << "." << oglContext->GetGLMinorVersion();

	//	Show the window and make it the front and focused one
	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	//	Center the window
	MoveWindow(hWnd, 0, 0, winWidth, winHeight, true);	//	TODO Center
	//	Finished
	LOG(INFO) << "Created window " << winWidth << "x" << winHeight << "x24" 
		<< " in " << (fullscreen ? "fullscreen" : "windowed") << " mode";
	return true;
}

void KillOGLWindow()
{
	//	Switch out of fullscreen if we were fullscreened
	if (fullscreen)
	{
		ChangeDisplaySettings(nullptr, 0);
		ShowCursor(true);
	}
	//	Destroy our context
	delete oglContext;
	oglContext = nullptr;
	//	Destroy the window
	if (hWnd && !DestroyWindow(hWnd))
	{
		LOG(ERROR) << "Failed to destroy window";
		hWnd = nullptr;
	}
	//	Unregister our class
	if (!UnregisterClass(DV_CLASS_NAME, hInstance))
	{
		LOG(ERROR) << "Failed to unregister window class";
		hInstance = nullptr;
	}
	LOG(INFO) << "Window destroyed";
}
