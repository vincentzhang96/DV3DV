#pragma once
#include "Application.h"
#include "3d/ShaderManager.h"
#include "3d/TextRenderer.h"

class UserInterface;

class UIScreen
{

public:
	DivinitorApp* _app;
	UserInterface* _ui;
	dv3d::TextRenderer* _text;
	explicit UIScreen(DivinitorApp* app);

	virtual ~UIScreen() {}

	virtual void Init() = 0;

	virtual void Draw(float deltaT) = 0;
};


class UserInterface
{

	GLuint _screenFbo;
	GLuint _screenFboColorTex;
	GLuint _screenRbo;
	GLuint _screenQuadVao;

	dv3d::GLPROGHANDLE _screenFboProg;

	glm::ivec2 _size;

	UIScreen* _activeScreen;
	UIScreen* _prevScreen;

	bool _newScreen;

	glm::fmat4 projView;

public:
	DivinitorApp* _app;

	explicit UserInterface(DivinitorApp* app);
	~UserInterface();

	void Init();

	void Draw(float deltaTime);

	void Resize(int width, int height);

	glm::ivec2 GetScreenSize() const;

	void SetActiveScreen(UIScreen* newScreen);

	void InvalidateOldScreen();

	const float* GetProjViewMatrixPtr();
};

class UiElement
{
public:
	const UIScreen* _ui;
	glm::fvec2 _pos;
	glm::fvec2 _size;
	explicit UiElement(UIScreen* parent) :
		_ui(parent)
	{
	}

	virtual ~UiElement() {}

	virtual void Draw(float deltaT) = 0;
};

