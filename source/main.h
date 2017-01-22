#pragma once
#ifndef H_DV3DV_MAIN
#define H_DV3DV_MAIN

struct DV3DVConfig
{
	GLint winWidth;
	GLint winHeight;
	bool fullscreen;
	bool console;
};

/// Main entry point
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow);

/// Windows events
LRESULT CALLBACK WndProc(HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

bool CreateOGLWindow(LPCWSTR winTitle,
	int winWidth, 
	int winHeight, 
	bool fullscreenWindow);

void KillOGLWindow();

///	Handler for window activation (unminimize)
void OnWindowActive();

///	Handler for window inactivation (minimize)
void OnWindowInactive();


void OnWindowResize(int newWidth, int newHeight);


void OnKeyDown(int keyCode);


void OnKeyUp(int keyCode);

void _TrySetupFullscreen(int winWidth, int winHeight, DWORD& dwExStyle, DWORD& dwStyle);

inline bool _DoMainLoop();

void _SetUpLogger();

bool _CreateUserDir();

bool _LoadConfig(DV3DVConfig& config);

bool _WriteConfig(DV3DVConfig& config);

void _ParseCommandLineFlag(DV3DVConfig& config, LPWSTR* argv, int argc, int i);

void _LoadIcon(WNDCLASSEX& wc);

#endif
