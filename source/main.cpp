#include "stdafx.h"
#include "main.h"
#include "OpenGLContext.h"

INITIALIZE_EASYLOGGINGPP

OpenGLContext* oglContext = nullptr;
HWND hWnd = nullptr;
HINSTANCE hInstance = nullptr;

bool active;
bool fullscreen;

void OnWindowActive()
{
	auto wasActive = active;
	active = true;
	if (active != wasActive)
	{
		LOG(DEBUG) << "Window unminimized";
	}
}

void OnWindowInactive()
{
	auto wasActive = active;
	active = false;
	if (active != wasActive)
	{
		LOG(DEBUG) << "Window minimized";
	}
}

void OnWindowResize(int newWidth, int newHeight)
{
	oglContext->ResizeWindow(newWidth, newHeight);
	LOG(INFO) << "Window resized to " << newWidth << "x" << newHeight;

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

bool _ShowConsole()
{
	if (!AllocConsole())
	{
		LOG(ERROR) << "Failed to allocate console";
		return false;
	}
	FILE* newStd;
	freopen_s(&newStd, "CONOUT$", "w", stdout);
	return true;
}

int WINAPI WinMain(HINSTANCE hInstance,	
                   HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, 
                   int nCmdShow)
{
	//	Disable FATAL app aborts
	el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);

	//	Options
	auto optShowConsole = true;

	//	Handle command line
	LPWSTR* argv;
	int argc;
	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv == nullptr)
	{
		MessageBox(nullptr, 
		           L"Failed to parse command line.", 
		           L"DV3DV Startup Error", 
		           MB_OK | MB_ICONERROR);
		return -1;
	}
	//	Handle arguments
	for (auto i = 0; i < argc; ++i)
	{
		MessageBox(nullptr, argv[i], L"DV3DV", MB_OK);
	}
	LocalFree(argv);
	//	Create console if necessary
	if (optShowConsole && !_ShowConsole())
	{
		if (!MessageBox(nullptr, 
		                L"Failed to create console window. Do you wish to continue?", 
		                L"DV3DV Startup Error", 
		                MB_YESNO | MB_ICONEXCLAMATION))
		{
			return 0;
		}
	}

	LOG(INFO) << "Starting...";

	if (!CreateOGLWindow(L"DV3DV", 1600, 900, false))
	{
		LOG(ERROR) << "Failed to create window, stopping";
		return 0;
	}

	MSG msg;
	auto exitLoop = false;

	//	Main loop
	LOG(INFO) << "Entering main loop";
	while (!exitLoop) 
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			//	Handle window messages
			if (msg.message == WM_QUIT)
			{
				//	Quit requested

				//	TODO have this signal into the program instead of killing immediately
				exitLoop = true;
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
	}
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
			OnWindowResize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);	//	TODO load icon
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = DV_CLASS_NAME;
	
	if (!RegisterClass(&wc))
	{
		LOG(ERROR) << "Window registration failed";
		MessageBox(nullptr, 
		           L"Failed to register window.", 
		           L"DV3DV Startup Error", 
		           MB_OK | MB_ICONERROR);
		return false;
	}

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
	if (fullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
		ShowCursor(false);
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW;
	}
	AdjustWindowRectEx(&windowRect, dwStyle, false, dwExStyle);
	hWnd = CreateWindowEx(dwExStyle,
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
		MessageBox(nullptr, 
		           L"Failed to create window", 
		           L"DV3DV Error", 
		           MB_OK | MB_ICONERROR);
		return false;
	}

	if (!oglContext->CreateContext(hWnd))
	{
		LOG(ERROR) << "Context creation failed";
		KillOGLWindow();
		MessageBox(nullptr,
			L"Failed to create context",
			L"DV3DV Error",
			MB_OK | MB_ICONERROR);
		return false;
	}

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	MoveWindow(hWnd, 0, 0, winWidth, winHeight, true);	//	TODO Center
	LOG(INFO) << "Created window " << winWidth << "x" << winHeight << "x24" 
		<< " in " << (fullscreen ? "fullscreen" : "windowed") << " mode";
	return true;
}

void KillOGLWindow()
{
	if (fullscreen)
	{
		ChangeDisplaySettings(nullptr, 0);
		ShowCursor(true);
	}
	delete oglContext;
	oglContext = nullptr;
	if (hWnd && !DestroyWindow(hWnd))
	{
		//	TODO failed to release window
		hWnd = nullptr;
	}
	if (!UnregisterClass(DV_CLASS_NAME, hInstance))
	{
		//	TODO failed to unregister class
		hInstance = nullptr;
	}
	LOG(INFO) << "Window destroyed";
}
