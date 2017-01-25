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
	OpenGLContext();
	~OpenGLContext();

	bool CreateContext(HWND hWnd);
	void ResizeWindow(int newWidth, int newHeight);

	void PreRender();
	void PostRender();

	GLint GetGLMajorVersion() const
	{
		return _glMajorVer;
	}

	GLint GetGLMinorVersion() const
	{
		return _glMinorVer;
	}

	GLint GetWindowWidth() const
	{
		return _windowWidth;
	}

	GLint GetWindowHeight() const
	{
		return _windowHeight;
	}
};

