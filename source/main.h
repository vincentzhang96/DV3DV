#pragma once
#ifndef H_DV3DV_MAIN
#define H_DV3DV_MAIN

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
	int bitdepth, 
	bool fullscreenWindow);

void KillOGLWindow();

///	Handler for window activation (unminimize)
void OnWindowActive();

///	Handler for window inactivation (minimize)
void OnWindowInactive();


void OnWindowResize(int newWidth, int newHeight);


void OnKeyDown(int keyCode);


void OnKeyUp(int keyCode);

#endif
