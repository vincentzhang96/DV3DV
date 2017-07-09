#include "../stdafx.h"
#include "UserInterface.h"
#include "UiBootstrap.h"

UserInterface::UserInterface(DivinitorApp* app)
	: _size(0, 0), _app(app)
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, GLBUFFEROFFSETZERO);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, GLBUFFEROFFSET(sizeof(GLfloat) * 3));
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
	glDepthFunc(GL_LEQUAL);

	//	Draw screen
	if (_activeScreen)
	{
		if (_newScreen)
		{
			_activeScreen->Init();
			_activeScreen->Resize(_size.x, _size.y);
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
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, GLBUFFEROFFSETZERO);
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

		if (_activeScreen)
		{
			_activeScreen->Resize(width, height);
		}
	}
}

glm::ivec2 UserInterface::GetScreenSize() const
{
	return _size;
}

void UserInterface::SetActiveScreen(UiScreen* newScreen)
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

bool UserInterface::HandleKeyPress(int keycode)
{
	//	TODO
	if (_activeScreen)
	{
		return _activeScreen->HandleKeyPress(keycode);
	}
	return false;
}

bool UserInterface::HandleKeyRelease(int keycode)
{
	//	TODO
	if (_activeScreen)
	{
		return _activeScreen->HandleKeyRelease(keycode);
	}
	return false;
}

bool UserInterface::HandleMouseClick(int x, int y, int mousebutton)
{
	//	TODO
	if (_activeScreen)
	{
		return _activeScreen->HandleMouseClick(x, y, mousebutton);
	}
	return false;
}

bool UserInterface::HandleMouseRelease(int x, int y, int mousebutton)
{
	//	TODO
	if (_activeScreen)
	{
		return _activeScreen->HandleMouseRelease(x, y, mousebutton);
	}
	return false;
}

glm::fvec2 UiElementAlignment::Position(AnchorX xAnchor, AnchorY yAnchor, glm::fvec2 pos, glm::fvec2 size, glm::fvec2 parentSize)
{
	return { PositionX(xAnchor, pos.x, size.x, parentSize.x), PositionY(yAnchor, pos.y, size.y, parentSize.y) };
}

glm::fvec2 UiElementAlignment::Position(AnchorX xAnchor, AnchorY yAnchor, glm::fvec2 pos, glm::fvec2 size, UiScreen* parent)
{
	return Position(xAnchor, yAnchor, pos, size, parent->_screenSize);
}

glm::fvec2 UiElementAlignment::Position(AnchorX xAnchor, AnchorY yAnchor, glm::fvec2 pos, UiElement* element, UiScreen* parent)
{
	auto ret = Position(xAnchor, yAnchor, pos, element->_size, parent);
	element->_pos = ret;
	return ret;
}

float UiElementAlignment::PositionX(AnchorX xAnchor, float xPos, float xSize, float xParentSize)
{
	switch (xAnchor)
	{
	case XCENTER:
		return (xParentSize / 2.0) - (xSize / 2.0) + xPos;
	case XRIGHT:
		return xParentSize - xSize - xPos;
	case XLEFT:
	default:
		return xPos;
	}
}

float UiElementAlignment::PositionX(AnchorX xAnchor, float xPos, UiElement* element, UiScreen* parent)
{
	auto ret = PositionX(xAnchor, xPos, element->_size.x, parent->_screenSize.x);
	element->_pos.x = ret;
	return ret;
}

float UiElementAlignment::PositionY(AnchorY yAnchor, float yPos, float ySize, float yParentSize)
{
	switch (yAnchor)
	{
	case YCENTER:
		return (yParentSize / 2.0) - (ySize / 2.0) + yPos;
	case YTOP:
		return yParentSize - yPos - ySize;
	case YBOTTOM:
	default:
		return yPos;
	}
}

float UiElementAlignment::PositionY(AnchorY yAnchor, float yPos, UiElement* element, UiScreen* parent)
{
	auto ret = PositionY(yAnchor, yPos, element->_size.y, parent->_screenSize.y);
	element->_pos.y = ret;
	return ret;
}

UiScreen::UiScreen(DivinitorApp* app) : _app(app)
{
	_ui = _app->_userInterface;
	_text = _app->_textRenderer;
}

void UiScreen::DrawUiElements(float deltaT)
{
	for (auto iter = _elements.begin(); iter != _elements.end(); ++iter)
	{
		iter->get()->Draw(deltaT);
	}
}

void UiScreen::ProcessUiElements()
{
	typedef std::unique_ptr<UiElement> UEPTR;
	std::sort(_elements.begin(), _elements.end(), [](UEPTR &a, UEPTR &b)
	{
		return a->_zLevel < b->_zLevel;
	});
}

void UiScreen::ClearUiElements()
{
	_elements.clear();
}
