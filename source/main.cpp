#include "stdafx.h"
#include "main.h"
#include "3d/OpenGLContext.h"

#include "ppac/PhoenixPAC.h"
#include "dn/pak/DnPak.h"
#include "ResourceManager.h"
#include "Application.h"

INITIALIZE_EASYLOGGINGPP
INIT_PPAC_LOGGER
INIT_DNPAK_LOGGER
INIT_FMOD_LOGGER

DV3DVConfig config;
OpenGLContext* oglContext = nullptr;
HWND hWnd = nullptr;
HINSTANCE hInstance = nullptr;
MSG msg;
LPSTR userDataDir;

glm::ivec2 _mouseCoords;

#define USER_DATA_DIRW L"./userdata/"
#define USER_DATA_DIR_FILEW(SUBPATH) USER_DATA_DIRW SUBPATH

bool active;
bool fullscreen;

using json = nlohmann::json;

std::unique_ptr<resman::ResourceManager> mResManager;

std::unique_ptr<DivinitorApp> mApp;

#define TPUID_ICON ppac::TPUID(0x0205, 0x0000, 0x00000001)
#define TPUID_SPLASH ppac::TPUID(0x0206, 0x0000, 0x00000001)
#define TPUID_SPLASH_VERT_SHDR ppac::TPUID(0x0107, 0x0000, 0x00000001)
#define TPUID_SPLASH_FRAG_SHDR ppac::TPUID(0x0108, 0x0000, 0x00000001)

void OnWindowActive()
{
	auto wasActive = active;
	active = true;
	if (active != wasActive)
	{
		LOG(TRACE) << "Window unminimized";
		//	TODO Notify
	}
}

void OnWindowInactive()
{
	auto wasActive = active;
	active = false;
	if (active != wasActive)
	{
		LOG(TRACE) << "Window minimized";
		//	TODO Notify
	}
}

void OnWindowResize(int newWidth, int newHeight)
{
	if (oglContext == nullptr)
	{
		//	The context no longer exists, don't bother
		return;
	}
	oglContext->ResizeWindow(newWidth, newHeight);
	LOG(TRACE) << "Window resized to " << newWidth << "x" << newHeight;

	//	TODO resetup stuff
	if (mApp)
	{
		mApp->OnViewportResized(newWidth, newHeight);
	}
}

void OnKeyDown(int keyCode)
{
	//	TODO
	if (mApp)
	{
		mApp->OnKeyPressed(keyCode);
	}
}

void OnKeyUp(int keyCode)
{
	//	TODO temporary quit mechanism
	if (keyCode == VK_ESCAPE)
	{
		PostQuitMessage(0);
	}
	//	TODO
	if (mApp)
	{
		mApp->OnKeyReleased(keyCode);
	}
}

void OnMouseMove(int x, int y, int mouseButton)
{
	_mouseCoords.x = x;
	_mouseCoords.y = oglContext->GetWindowHeight() - y;
	if (mApp)
	{
		mApp->OnMouseMoved(_mouseCoords.x, _mouseCoords.y, mouseButton);
	}
}

void OnMouseButtonDown(int mouseButton)
{
	if (mApp)
	{
		mApp->OnMouseButtonPressed(_mouseCoords.x, _mouseCoords.y, mouseButton);
	}
}

void OnMouseButtonUp(int mouseButton)
{
	if (mApp)
	{
		mApp->OnMouseButtonReleased(_mouseCoords.x, _mouseCoords.y, mouseButton);
	}
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
		mApp->Draw();
		oglContext->PostRender();
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

bool _CreateUserDir()
{
	if (GetFileAttributesW(USER_DATA_DIRW) == 0xFFFFFFFF)
	{
		if (CreateDirectoryW(USER_DATA_DIRW, nullptr) == 0)
		{
			MessageBoxW(nullptr,
			            L"Failed to create user data directory at " USER_DATA_DIRW,
			            L"DV3DV Startup Error",
			            MB_OK | MB_ICONERROR);
			return false;
		}
	}

	if (GetFileAttributesW(USER_DATA_DIR_FILEW(L"config")) == 0xFFFFFFFF)
	{
		if (CreateDirectoryW(USER_DATA_DIR_FILEW(L"config"), nullptr) == 0)
		{
			MessageBoxW(nullptr,
			            L"Failed to create user config directory at " USER_DATA_DIR_FILEW(L"config"),
			            L"DV3DV Startup Error",
			            MB_OK | MB_ICONERROR);
			return false;
		}
	}
	return true;
}

bool _LoadConfig(DV3DVConfig& config)
{
	config.winWidth = 1600;
	config.winHeight = 900;
	config.console = true;
	config.fullscreen = false;
	config.vsync = true;
	//	Check that the config file exists before attempting to load it
	auto dwAttrib = GetFileAttributesW(USER_DATA_DIR_FILEW(L"config/config.json"));
	if (dwAttrib == 0xFFFFFFFF)
	{
		//	Configuration does not exist, so we'll create it and save it
		if (!_WriteConfig(config))
		{
			return false;
		}
	}
	//	Read it in
	std::ifstream istream(USER_DATA_DIR_FILEW(L"config/config.json"));
	json cfgJson;
	istream >> cfgJson;
	istream.close();
	if (cfgJson.count("width") == 1)
	{
		config.winWidth = cfgJson["width"].get<int>();
	}
	if (cfgJson.count("height") == 1)
	{
		config.winHeight = cfgJson["height"].get<int>();
	}
	if (cfgJson.count("fullscreen") == 1)
	{
		config.fullscreen = cfgJson["fullscreen"].get<bool>();
	}
	if (cfgJson.count("console") == 1)
	{
		config.console = cfgJson["console"].get<bool>();
	}
	if (cfgJson.count("vsync") == 1)
	{
		config.vsync = cfgJson["vsync"].get<bool>();
	}
	LOG(TRACE) << "Config loaded";
	return true;
}

bool _WriteConfig(DV3DVConfig& config)
{
	auto cfgFile = CreateFileW(USER_DATA_DIR_FILEW(L"config/config.json"),
	                                                                     GENERIC_WRITE,
	                                                                     0,
	                                                                     nullptr,
	                                                                     OPEN_ALWAYS,
	                                                                     FILE_ATTRIBUTE_NORMAL,
	                                                                     nullptr);
	if (cfgFile == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(nullptr,
		            L"Failed to create user config at " USER_DATA_DIR_FILEW(L"config/config.json"),
		            L"DV3DV Startup Error",
		            MB_OK | MB_ICONERROR);
		return false;
	}
	json cfgJson = 
	{
		{"width", config.winWidth},
		{"height", config.winHeight},
		{"fullscreen", config.fullscreen},
		{"console", config.console},
		{"vsync", config.vsync}
	};
	//	Truncate file
	SetEndOfFile(cfgFile);
	std::string jsonStr = cfgJson.dump(4);
	auto buf = jsonStr.c_str();
	DWORD dwBytesWritten;
	auto success = WriteFile(cfgFile,
	                         buf,
	                         DWORD(strlen(buf)),
	                         &dwBytesWritten,
	                         nullptr
	);
	if (success == 0)
	{
		MessageBoxW(nullptr,
		            L"Failed to write user config at " USER_DATA_DIR_FILEW(L"config/config.json"),
		            L"DV3DV Startup Error",
		            MB_OK | MB_ICONERROR);
		CloseHandle(cfgFile);
		return false;
	}
	CloseHandle(cfgFile);
	LOG(TRACE) << "Config saved";
	return true;
}

void _ParseCommandLineFlag(DV3DVConfig& config, LPWSTR* argv, int argc, int i)
{
	if (lstrcmpiW(L"/console", argv[i]) == 0)
	{
		config.console = true;
	}
	else if (lstrcmpiW(L"/w", argv[i]) == 0 && i + 1 < argc)
	{
		config.winWidth = _wtoi(argv[i + 1]);
	}
	else if (lstrcmpiW(L"/h", argv[i]) == 0 && i + 1 < argc)
	{
		config.winHeight = _wtoi(argv[i + 1]);
	}
	else if (lstrcmpiW(L"/fullscreen", argv[i]) == 0)
	{
		config.fullscreen = true;
	}
	else if (lstrcmpiW(L"/windowed", argv[i]) == 0)
	{
		config.fullscreen = false;
	}
	else if (lstrcmpiW(L"/vsync", argv[i]) == 0)
	{
		config.vsync = true;
	}
	else if (lstrcmpiW(L"/novsync", argv[i]) == 0)
	{
		config.vsync = false;
	}
}

void _drawSplash()
{
	auto splashTex = mApp->_textureManager->LoadAndGet(TPUID_SPLASH);
	auto hSplashShdr = mApp->_shaderManager->NewProgram();
	mApp->_shaderManager->AttachAndCompileShader(hSplashShdr, TPUID_SPLASH_VERT_SHDR);
	mApp->_shaderManager->AttachAndCompileShader(hSplashShdr, TPUID_SPLASH_FRAG_SHDR);
	bool shdrOk = mApp->_shaderManager->LinkAndFinishProgram(hSplashShdr);
	if (splashTex.first && shdrOk)
	{
		//	Textured quad
		GLfloat quadHalfSizeX = 512.0F;
		GLfloat quadHalfSizeY = 512.0F;
		//	Convert to NDC
		quadHalfSizeX /= oglContext->GetWindowWidth();
		quadHalfSizeY /= oglContext->GetWindowHeight();

		GLfloat verts[] = {
			-quadHalfSizeX, -quadHalfSizeY, 0.0F, 0.0F, 0.0F,
			quadHalfSizeX, -quadHalfSizeY, 0.0F, 1.0F, 0.0F,
			quadHalfSizeX, quadHalfSizeY, 0.0F, 1.0F, 1.0F,
			-quadHalfSizeX, quadHalfSizeY, 0.0F, 0.0F, 1.0F
		};
		GLuint indx[] = {
			0, 1, 2, 3
		};
		GLuint vao;
		glGenVertexArrays(1, &vao);
		if (vao == GL_INVALID_VALUE)
		{
			LOG(WARNING) << "Failed to create VAO";
			return;
		}
		GLuint vbo[2];
		glGenBuffers(2, vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, GLBUFFEROFFSETZERO);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, GLBUFFEROFFSET_F(3));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indx), &indx[0], GL_STATIC_DRAW);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		oglContext->PreRender();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		auto prog = mApp->_shaderManager->Get(hSplashShdr);
		glUseProgram(prog);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, splashTex.second);
		glProgramUniform1i(prog, 2, 0);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, GLBUFFEROFFSETZERO);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
		oglContext->PostRender();

		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(2, vbo);
		mApp->_textureManager->Unload(splashTex.first);
		mApp->_shaderManager->Unload(hSplashShdr);
	}
	else
	{
		LOG(WARNING) << "No splash texture found";
	}
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
	_SetUpLogger();
	_CreateUserDir();
	//	Options
	ZeroMemory(&config, sizeof(DV3DVConfig));
	_LoadConfig(config);

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
	for (auto i = 0; i < argc; ++i)
	{
		_ParseCommandLineFlag(config, argv, argc, i);
	}
	//	Don't need argv anymore, free
	LocalFree(argv);

	//	Create console if necessary
	if (config.console && !_ShowConsole())
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
	LOG(INFO) << "Divinitor 3D Viewer v0.1.0";
	LOG(INFO) << "Built " << __TIMESTAMP__;
	LOG(INFO) << "Starting...";
	//	Init asset manager
	mResManager = std::make_unique<resman::ResourceManager>();
	//	Load init
	mResManager->_ppacManager.LoadPPAC(L"init.ppac");
	//	Create the window
	if (!CreateOGLWindow(L"DV3DV", config.winWidth, config.winHeight, config.fullscreen))
	{
		LOG(ERROR) << "Failed to create window, stopping";
		return 0;
	}
	//	Init application
	mApp = std::make_unique<DivinitorApp>(mResManager.get());
	//	Fire off a resize since we missed the window resize
	mApp->OnViewportResized(oglContext->GetWindowWidth(), oglContext->GetWindowHeight());

	// Starting with VSync on does weirdness with CPU usage, so push a few frames through
	if (config.vsync) {
		wglSwapIntervalEXT(1);
		for (auto i = 0; i < 60; ++i)
		{
			oglContext->PreRender();
			oglContext->PostRender();
		}
		wglSwapIntervalEXT(0);
		for (auto i = 0; i < 60; ++i)
		{
			oglContext->PreRender();
			oglContext->PostRender();
		}
		wglSwapIntervalEXT(1);
	}

	//	Render splash screen
	_drawSplash();
	//	Load data directory
	mResManager->_ppacManager.LoadDir(L"data");
	//	Prep for main loop
	auto exitLoop = false;
	mApp->FirstFrameInit();

	//	Main loop
	LOG(INFO) << "Entering main loop";
	while (!exitLoop)
	{
		exitLoop = _DoMainLoop();
	}
	//	Destroy window
	KillOGLWindow();
	//	Shut down managers
	//	TODO TEMP
	mApp.reset();
	mResManager.reset();

	LOG(INFO) << "Shutdown complete";
	Sleep(1000);
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
	case WM_MOUSEMOVE:
		{
			OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
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
		devScreenSettings.dmBitsPerPel = 32;
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

void _LoadIcon(WNDCLASSEX& wc)
{
	auto iconData = mResManager->GetResource(TPUID_ICON);
	if (iconData)
	{
		HWND hConsoleWindow = GetConsoleWindow();
		int largeIconXSz = GetSystemMetrics(SM_CXICON);
		int largeIconYSz = GetSystemMetrics(SM_CYICON);
		int smallIconXSz = GetSystemMetrics(SM_CXSMICON);
		int smallIconYSz = GetSystemMetrics(SM_CYSMICON);
		uint8_t* buf = iconData->data();
		//	Fix icon for memory rep
		uint16_t count = *reinterpret_cast<uint16_t*>(&buf[4]);
		for (auto i = 1; i < count; ++i)
		{
			memcpy(buf + 6 + (i * 0xE), buf + 6 + (i * 0x10), 0xE);
		}
		HICON hLgIcon = nullptr;
		int lgOffset = LookupIconIdFromDirectoryEx(buf, true, largeIconXSz, largeIconYSz, LR_DEFAULTCOLOR);
		if (lgOffset != 0)
		{
			hLgIcon = CreateIconFromResourceEx(buf + lgOffset, 0, true, 0x30000, largeIconXSz, largeIconYSz, LR_DEFAULTCOLOR);
			wc.hIcon = hLgIcon;
			if (hConsoleWindow)
			{
				SendMessageW(hConsoleWindow, WM_SETICON, ICON_BIG, LPARAM(hLgIcon));
			}
		}
		else
		{
			wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
		}
		HICON hSmIcon;
		int smOffset = LookupIconIdFromDirectoryEx(buf, true, smallIconXSz, smallIconYSz, LR_DEFAULTCOLOR);
		if (smOffset != 0)
		{
			hSmIcon = CreateIconFromResourceEx(buf + smOffset, 0, true, 0x30000, smallIconXSz, smallIconYSz, LR_DEFAULTCOLOR);
			wc.hIconSm = hSmIcon;
			if (hConsoleWindow)
			{
				SendMessageW(hConsoleWindow, WM_SETICON, ICON_SMALL, LPARAM(hLgIcon));
			}
		}
		else
		{
			wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
		}
	}
	else
	{
		wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
		wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
	}
}

bool CreateOGLWindow(LPCWSTR winTitle,
                     int winWidth,
                     int winHeight,
                     bool fullscreenWindow)
{
	WNDCLASSEX wc;
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
	//	Load icon
	_LoadIcon(wc);
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WNDPROC(WndProc);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = DV_CLASS_NAME;

	if (!RegisterClassEx(&wc))
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

	//	Rename console if present
	HWND hConWin = GetConsoleWindow();
	if (hConWin)
	{
		SetWindowText(hConWin, L"DV3DV Debug Console");
	}

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
