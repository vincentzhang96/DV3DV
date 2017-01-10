#pragma once

class OpenGLContext
{
private:
	int _windowWidth;
	int _windowHeight;
	bool _fullscreen;

protected:
	HDC _hDC = nullptr;
	HGLRC _hRC = nullptr;
	HWND _hWnd = nullptr;
	GLint _glMajorVer = -1;
	GLint _glMinorVer = -1;

public:
	OpenGLContext(void);
	~OpenGLContext(void);

	bool CreateContext(HWND hWnd);
	void ResizeWindow(int newWidth, int newHeight);

	void PreRender(void);
	void PostRender(void);

};
