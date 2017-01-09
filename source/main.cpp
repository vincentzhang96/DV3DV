#include "stdafx.h"
#include "main.h"

#define DV_CLASS_NAME L"OpenGL"

INITIALIZE_EASYLOGGINGPP

HDC hDC = nullptr;
HGLRC hRC = nullptr;
HWND hWnd = nullptr;
HINSTANCE hInstance;

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

	if (!CreateOGLWindow(L"DV3DV", 1600, 900, 32, false))
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
		//	TODO

		return 0;
	}
	case WM_KEYUP:
	{
		//	TODO

		return 0;
	}
	case WM_SIZE:
	{
		//	TODO

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
	int bitdepth, 
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
		devScreenSettings.dmBitsPerPel = bitdepth;
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

	static PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = bitdepth;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;

	hDC = GetDC(hWnd);
	if (!hDC)
	{
		LOG(ERROR) << "Failed to create OpenGL device";
		KillOGLWindow();
		MessageBox(nullptr, 
			L"Failed to create OpenGL device", 
			L"DV3DV Error", 
			MB_OK | MB_ICONERROR);
		return false;
	}
	pixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (!pixelFormat)
	{
		LOG(ERROR) << "Unable to find a suitable pixel format";
		KillOGLWindow();
		MessageBox(nullptr, 
			L"No suitable pixel format found", 
			L"DV3DV Error", 
			MB_OK | MB_ICONERROR);
		return false;
	}
	if (!SetPixelFormat(hDC, pixelFormat, &pfd))
	{
		LOG(ERROR) << "Failed to set pixel format";
		KillOGLWindow();
		MessageBox(nullptr, 
			L"Failed to set pixel format", 
			L"DV3DV Error", 
			MB_OK | MB_ICONERROR);
		return false;
	}
	hRC = wglCreateContext(hDC);
	if (!hRC)
	{
		LOG(ERROR) << "Failed to create OpenGL rendering context";
		KillOGLWindow();
		MessageBox(nullptr, 
			L"Failed to create OpenGL rendering context", 
			L"DV3DV Error", 
			MB_OK | MB_ICONERROR);
		return false;
	}
	if (!wglMakeCurrent(hDC, hRC))
	{
		LOG(ERROR) << "Failed to activate OpenGL rendering context";
		KillOGLWindow();
		MessageBox(nullptr, 
			L"Failed to activate OpenGL rendering context", 
			L"DV3DV Error", 
			MB_OK | MB_ICONERROR);
		return false;
	}
	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	MoveWindow(hWnd, 0, 0, winWidth, winHeight, true);	//	TODO Center
	LOG(INFO) << "Created window " << winWidth << "x" << winHeight << "x" << bitdepth 
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
	if (hRC)
	{
		if (!wglMakeCurrent(nullptr, nullptr))
		{
			//	TODO failed to release dc and rc
		}
		if (!wglDeleteContext(hRC))
		{
			//	TODO failed to release context
		}
		hRC = nullptr;
	}
	if (hDC && !ReleaseDC(hWnd, hDC))
	{
		//	TODO failed to release device context
		hDC = nullptr;
	}
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
