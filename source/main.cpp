#include "stdafx.h"
#include "main.h"
#include "OpenGLContext.h"

#include "ppac/PhoenixPAC.h"
#include "dn/pak/DnPak.h"

INITIALIZE_EASYLOGGINGPP
INIT_PPAC_LOGGER
INIT_DNPAK_LOGGER

OpenGLContext* oglContext = nullptr;
HWND hWnd = nullptr;
HINSTANCE hInstance = nullptr;
MSG msg;
LPSTR userDataDir;


#define USER_DATA_DIRW L"./userdata/"
#define USER_DATA_DIR_FILEW(SUBPATH) USER_DATA_DIRW SUBPATH

bool active;
bool fullscreen;

using json = nlohmann::json;

using PPACMANAGER = ppac::cPPACManager;

std::unique_ptr<PPACMANAGER> mPPACManager;

#define TPUID_ICON { 0x0205, 0x0000, 0x00000001 }
#define TPUID_SPLASH { 0x0206, 0x0000, 0x00000001 }

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
		{"console", config.console}
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
}

void _drawSplash()
{
	//	Normally the texture manager would handle the loading and registering and maintaining the texture for us
	//	But we're still in startup and have to get this splash drawn as fast as possible
	auto splashData = mPPACManager.get()->GetFileData(TPUID_SPLASH);
	if (splashData)
	{
#define ASSERT_NO_GL_ERROR assert(glGetError() == GL_NO_ERROR);
		GLenum err;
		auto dataVec = splashData->_data;
		uint32_t* asUint32 = reinterpret_cast<uint32_t*>(dataVec.data());
		uint32_t* itr = asUint32;
		uint32_t magic = *itr;
		if (magic != 0x20534444)
		{
			LOG(WARNING) << "Invalid splash texture: not a DDS";
			return;
		}
		itr += 1;
		DDS_HEADER header = *reinterpret_cast<DDS_HEADER*>(itr);
		if (header.dwSize != 124)
		{
			LOG(WARNING) << "Corrupt or invalid DDS: invalid dwSize, expected 124, got " << header.dwSize;
			return;
		}
		size_t bufSize = (header.dwMipMapCount > 1) ? header.dwPitchOrLinearSize * 2 : header.dwPitchOrLinearSize;
		std::unique_ptr<uint8_t[]> buf(new uint8_t[bufSize]);
		std::copy_n(&dataVec[128], bufSize, buf.get());
		size_t numComponenets = (header.ddspf.dwFourCC == DDS_FOURCC_DXT1) ? 3 : 4;
		GLuint glTexFormat;
		switch (header.ddspf.dwFourCC)
		{
		case DDS_FOURCC_DXT1:
			glTexFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			break;
		case DDS_FOURCC_DXT3:
			glTexFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			break;
		case DDS_FOURCC_DXT5:
			glTexFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		default:
			LOG(WARNING) << "Unknown FourCC " << header.ddspf.dwFourCC;
			return;
		}
		size_t blockSize = (glTexFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
		size_t offset = 0;
		size_t width = header.dwWidth;
		size_t height = header.dwHeight;
		GLuint splashTextureId;
		glGenTextures(1, &splashTextureId);
		glBindTexture(GL_TEXTURE_2D, splashTextureId);
		ASSERT_NO_GL_ERROR;
		size_t imgSize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, glTexFormat, width, height, 0, imgSize, buf.get());
		ASSERT_NO_GL_ERROR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		ASSERT_NO_GL_ERROR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		ASSERT_NO_GL_ERROR;

		glBindTexture(GL_TEXTURE_2D, 0);

		err = glGetError();
		if (err != GL_NO_ERROR)
		{
			LOG(WARNING) << glewGetErrorString(err) << err;
		}

		//	Shader
		std::string vertShaderSrc = R"(#version 430
		layout(location = 0) in vec3 position;
		layout(location = 1) in vec2 uvCoords;
		layout(location = 2) uniform sampler2D mtl;

		out vec2 textureCoords;

		void main()
		{
			gl_Position = vec4(position, 1.0);
			textureCoords = vec2(uvCoords.x, 1.0 - uvCoords.y);
		}
		)";
		std::string fragShaderSrc = R"(#version 430
		layout(location = 2) uniform sampler2D mtl;
		in vec2 textureCoords;
		out vec4 color;
		void main()
		{
			//color = vec4(textureCoords.xy, 0.0, 1.0);
			vec4 texColor = texture(mtl, textureCoords);
			if (texColor.a < 1.0 / 255.0)
			{
				discard;
			}
			color = texColor;
		}
		)";
		GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
		const char* vertShaderCstr = vertShaderSrc.c_str();
		glShaderSource(vertShader, 1, &vertShaderCstr, 0);
		glCompileShader(vertShader);

		GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		const char* fragShaderCstr = fragShaderSrc.c_str();
		glShaderSource(fragShader, 1, &fragShaderCstr, 0);
		glCompileShader(fragShader);

		GLint success;
		glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			GLchar infoLog[512];
			glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
			LOG(WARNING) << "Vertex shader compilation failed: " << infoLog;
			return;
		}
		glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			GLchar infoLog[512];
			glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
			LOG(WARNING) << "Vertex shader compilation failed: " << infoLog;
			return;
		}
		GLuint shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertShader);
		glAttachShader(shaderProgram, fragShader);
		glLinkProgram(shaderProgram);
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success) {
			GLchar infoLog[512];
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			LOG(WARNING) << "Shader linking failed: " << infoLog;
			return;
		}

		int vertLoc = glGetAttribLocation(shaderProgram, "position");
		int uvLoc = glGetAttribLocation(shaderProgram, "uvCoords");
		int texSamplerLoc = glGetUniformLocation(shaderProgram, "mtl");

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
		glEnableVertexAttribArray(vertLoc);
		glVertexAttribPointer(vertLoc, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid*)0);
		glEnableVertexAttribArray(uvLoc);
		glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid*)(sizeof(GLfloat) * 3));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indx), &indx[0], GL_STATIC_DRAW);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		oglContext->PreRender();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(shaderProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, splashTextureId);
		glProgramUniform1i(shaderProgram, texSamplerLoc, 0);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (void*)0);
		glBindVertexArray(0);
		glUseProgram(0);

		err = glGetError();
		if (err != GL_NO_ERROR)
		{
			LOG(WARNING) << glewGetErrorString(err) << err;
		}
		oglContext->PostRender();

		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(2, vbo);
		glDeleteTextures(1, &splashTextureId);
		glDeleteProgram(shaderProgram);
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
	DV3DVConfig config;
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
	//	Init PPAC asset manager
	//	We'll init the general asset manager after we get a window with a splash displayed on screen as fast as possible
	mPPACManager = std::make_unique<ppac::cPPACManager>();
	//	PPAC manager, load init first
	mPPACManager.get()->LoadPPAC(L"init.ppac");
	//	Create the window
	if (!CreateOGLWindow(L"DV3DV", config.winWidth, config.winHeight, config.fullscreen))
	{
		LOG(ERROR) << "Failed to create window, stopping";
		return 0;
	}
	//	Render splash screen
	_drawSplash();


	Sleep(10000);

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
	//	Shut down managers
	mPPACManager.reset();

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
	auto iconData = mPPACManager.get()->GetFileData(TPUID_ICON);
	if (iconData)
	{
		HWND hConsoleWindow = GetConsoleWindow();
		int largeIconXSz = GetSystemMetrics(SM_CXICON);
		int largeIconYSz = GetSystemMetrics(SM_CYICON);
		int smallIconXSz = GetSystemMetrics(SM_CXSMICON);
		int smallIconYSz = GetSystemMetrics(SM_CYSMICON);
		auto iconPpacData = iconData.get();
		uint8_t* buf = iconPpacData->_data.data();
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
		HICON hSmIcon = nullptr;
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
