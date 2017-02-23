#include "stdafx.h"
#include "UserInterface.h"
#include "ui/UiBootstrap.h"

UserInterface::UserInterface(DivinitorApp* app)
	: _app(app), _size(0, 0)
{
	_screenFbo = 0;
	_screenFboProg = 0;
	_screenFboColorTex = 0;
	_screenRbo = 0;
	_screenQuadVao = 0;
	_activeScreen = nullptr;
	_prevScreen = nullptr;
	_newScreen = false;
}

UserInterface::~UserInterface()
{
}

void UserInterface::Init()
{
	LOG(DEBUG) << "UI init";
	auto shdrMan = _app->_shaderManager;
	_screenFboProg = shdrMan->NewProgram();
	//	NDC w/ texture vert shader
	shdrMan->AttachAndCompileShader(_screenFboProg, ppac::TPUID(0x0107, 0x0100, 0x00000001));
	//	UI FBO frag shader
	shdrMan->AttachAndCompileShader(_screenFboProg, ppac::TPUID(0x0108, 0x0101, 0x00000001));
	if(!shdrMan->LinkAndFinishProgram(_screenFboProg))
	{
		LOG(WARNING) << "Failed to create UI quad shader";
		throw "Failed to create UI quad shader";
	}

	glGenVertexArrays(1, &_screenQuadVao);
	if (_screenQuadVao == GL_INVALID_VALUE)
	{
		LOG(WARNING) << "Failed to create UI quad VAO";
		throw "Failed to create UI quad VAO";
	}
	GLfloat verts[] = {
		-1.0F, -1.0F, 0.0F, 0.0F, 0.0F,
		1.0F, -1.0F, 0.0F, 1.0F, 0.0F,
		1.0F, 1.0F, 0.0F, 1.0F, 1.0F,
		-1.0F, 1.0F, 0.0F, 0.0F, 1.0F
	};
	GLuint indx[] = {
		0, 1, 2, 3
	};
	GLuint vbo[2];
	glGenBuffers(2, vbo);
	glBindVertexArray(_screenQuadVao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid*)(sizeof(GLfloat) * 3));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indx), &indx[0], GL_STATIC_DRAW);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//	Bootstrap UI
	SetActiveScreen(new UiBootstrap(_app));
}

void UserInterface::Draw(float deltaTime)
{
	
	glBindFramebuffer(GL_FRAMEBUFFER, _screenFbo);
	glClearColor(19.0F / 255.0F, 19.0F / 255.0F, 21.0F / 255.0F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, _size.x, _size.y);
	//	Draw screen
	if (_activeScreen)
	{
		if (_newScreen)
		{
			_activeScreen->Init();
			_newScreen = false;
		}
		_activeScreen->Draw(deltaTime);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	auto prog = _app->_shaderManager->Get(_screenFboProg);
	glUseProgram(prog);
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _screenFboColorTex);
	glProgramUniform1i(prog, 2, 0);
	glBindVertexArray(_screenQuadVao);
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void UserInterface::Resize(int width, int height)
{
	if (_size.x != width || _size.y != height)
	{
		_size.x = width;
		_size.y = height;
		LOG(DEBUG) << "UI resize";
		//	Update matrix

		projView = glm::ortho(0.0F, float(width), 0.0F, float(height));

		//	Delete old buffers
		if (_screenFbo)
		{
			glDeleteFramebuffers(1, &_screenFbo);
		}
		if (_screenRbo)
		{
			glDeleteRenderbuffers(1, &_screenRbo);
		}
		if (_screenFboColorTex)
		{
			glDeleteTextures(1, &_screenFboColorTex);
		}
		//	Generate FBO
		glGenFramebuffers(1, &_screenFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, _screenFbo);
		glGenTextures(1, &_screenFboColorTex);
		glBindTexture(GL_TEXTURE_2D, _screenFboColorTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _screenFboColorTex, 0);
		//	Generate RBO
		glGenRenderbuffers(1, &_screenRbo);
		glBindRenderbuffer(GL_RENDERBUFFER, _screenRbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		//	Attach
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _screenRbo);
		//	Verify
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			LOG(WARNING) << "Failed to complete UI framebuffer";
			throw "UI framebuffer incomplete";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

glm::ivec2 UserInterface::GetScreenSize() const
{
	return _size;
}

void UserInterface::SetActiveScreen(UIScreen* newScreen)
{
	_prevScreen = _activeScreen;
	_activeScreen = newScreen;
	_newScreen = true;
}

void UserInterface::InvalidateOldScreen()
{
	if (_prevScreen != nullptr)
	{
		delete _prevScreen;
		_prevScreen = nullptr;
	}
}

const float* UserInterface::GetProjViewMatrixPtr()
{
	return glm::value_ptr(projView);
}

UIScreen::UIScreen(DivinitorApp* app) : _app(app)
{
	_ui = _app->_userInterface;
	_text = _app->_textRenderer;
}
