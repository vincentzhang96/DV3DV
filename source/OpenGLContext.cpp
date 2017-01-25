#include "stdafx.h"
#include "OpenGLContext.h"



OpenGLContext::OpenGLContext()
{
	_windowWidth = 1600;
	_windowHeight = 900;
	_fullscreen = false;
}

OpenGLContext::~OpenGLContext()
{
	if (_fullscreen)
	{
		ChangeDisplaySettings(nullptr, 0);
		ShowCursor(true);
	}
	if (_hRC)
	{
		if (!wglMakeCurrent(nullptr, nullptr))
		{
			//	TODO failed to release dc and rc
		}
		if (!wglDeleteContext(_hRC))
		{
			//	TODO failed to release context
		}
	}
	if (_hDC && !ReleaseDC(_hWnd, _hDC))
	{
		//	TODO failed to release device context
		_hDC = nullptr;
	}
}

bool OpenGLContext::CreateContext(HWND hWnd)
{
	_hWnd = hWnd;
	_hDC = GetDC(hWnd);
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;

	_hDC = GetDC(hWnd);
	if (!_hDC)
	{
		LOG(ERROR) << "Failed to create OpenGL device";
		return false;
	}
	GLuint pixelFormat = ChoosePixelFormat(_hDC, &pfd);
	if (!pixelFormat)
	{
		LOG(ERROR) << "Unable to find a suitable pixel format";
		return false;
	}
	if (!SetPixelFormat(_hDC, pixelFormat, &pfd))
	{
		LOG(ERROR) << "Failed to set pixel format";
		return false;
	}
	HGLRC tempContext = wglCreateContext(_hDC);
	if (!tempContext)
	{
		LOG(ERROR) << "Failed to create temporary OpenGL rendering context";
		return false;
	}
	if (!wglMakeCurrent(_hDC, tempContext))
	{
		LOG(ERROR) << "Failed to activate temporary OpenGL rendering context";
		return false;
	}
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		LOG(ERROR) << "Failed to init GLEW: " << glewGetErrorString(err);
		return false;
	}
	//	Get the highest available OpenGL version
	glGetIntegerv(GL_MAJOR_VERSION, &_glMajorVer);
	glGetIntegerv(GL_MINOR_VERSION, &_glMinorVer);
	//	Minimum requirement is OpenGL v4.3
	if (_glMajorVer < 4 || (_glMajorVer == 4 && _glMinorVer < 3))
	{
		LOG(ERROR) << "Unsupported OpenGL version " << _glMajorVer << "." << _glMinorVer;
		return false;
	}
	GLint attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, _glMajorVer,
		WGL_CONTEXT_MINOR_VERSION_ARB, _glMinorVer,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};
	if (wglewIsSupported("WGL_ARB_create_context") == 1)
	{
		_hRC = wglCreateContextAttribsARB(_hDC, nullptr, attribs);
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(tempContext);
		wglMakeCurrent(_hDC, _hRC);
	}
	else
	{
		_hRC = tempContext;
	}
	return true;
}

void OpenGLContext::ResizeWindow(int newWidth, int newHeight)
{
	_windowWidth = newWidth;
	_windowHeight = newHeight;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void OpenGLContext::PreRender()
{
	glViewport(0, 0, _windowWidth, _windowHeight);
	glClearColor(19.0F / 255.0F, 19.0F / 255.0F, 21.0F / 255.0F, 0.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void OpenGLContext::PostRender()
{
	SwapBuffers(_hDC);
}
