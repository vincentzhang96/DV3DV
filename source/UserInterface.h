#pragma once
#include "Application.h"
#include "3d/ShaderManager.h"
#include "3d/TextRenderer.h"

class UserInterface;
class UiElement;
class UiScreen;

class UiElement
{
public:
	const UiScreen* _ui;
	glm::fvec2 _pos;
	glm::fvec2 _size;
	GLfloat _zLevel;
	explicit UiElement(UiScreen* parent) :
		_ui(parent)
	{
		_zLevel = 0;
	}

	virtual ~UiElement() {}

	virtual void Draw(float deltaT) = 0;
};

class UiScreen
{

public:
	typedef std::vector<std::unique_ptr<UiElement>> ElementList;
	DivinitorApp* _app;
	UserInterface* _ui;
	dv3d::TextRenderer* _text;
	glm::fvec2 _screenSize;
	ElementList _elements;

	explicit UiScreen(DivinitorApp* app);

	virtual ~UiScreen() {}

	virtual void Init() = 0;

	virtual void Resize(int width, int height)
	{
		_screenSize.x = width;
		_screenSize.y = height;
	}

	virtual void Draw(float deltaT) = 0;

	void DrawUiElements(float deltaT);

	void ProcessUiElements();

	virtual bool HandleKeyPress(int keycode)
	{
		return false;
	}

	virtual bool HandleKeyRelease(int keycode)
	{
		return false;
	}

	virtual bool HandleMouseClick(int x, int y, int mousebutton)
	{
		return false;
	}

	virtual bool HandleMouseRelease(int x, int y, int mousebutton)
	{
		return false;
	}
};


class UserInterface
{

	GLuint _screenFbo;
	GLuint _screenFboColorTex;
	GLuint _screenRbo;
	GLuint _screenQuadVao;

	dv3d::GLPROGHANDLE _screenFboProg;

	glm::ivec2 _size;

	UiScreen* _activeScreen;
	UiScreen* _prevScreen;

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

	void SetActiveScreen(UiScreen* newScreen);

	void InvalidateOldScreen();

	const float* GetProjViewMatrixPtr();

	bool HandleKeyPress(int keycode);

	bool HandleKeyRelease(int keycode);

	bool HandleMouseClick(int x, int y, int mousebutton);

	bool HandleMouseRelease(int x, int y, int mousebutton);
};

namespace UiElementAlignment
{
	enum AnchorX
	{
		XLEFT,
		XCENTER,
		XRIGHT
	};
	enum AnchorY
	{
		YTOP,
		YCENTER,
		YBOTTOM
	};

	glm::fvec2 Position(AnchorX xAnchor, AnchorY yAnchor, glm::fvec2 pos, glm::fvec2 size, glm::fvec2 parentSize);
	glm::fvec2 Position(AnchorX xAnchor, AnchorY yAnchor, glm::fvec2 pos, glm::fvec2 size, UiScreen* parent);
	glm::fvec2 Position(AnchorX xAnchor, AnchorY yAnchor, glm::fvec2 pos, UiElement* element, UiScreen* parent);
	float PositionX(AnchorX xAnchor, float xPos, float xSize,float xParentSize);
	float PositionX(AnchorX xAnchor, float xPos, UiElement* element, UiScreen* parent);
	float PositionY(AnchorY yAnchor, float yPos, float ySize, float yParentSize);
	float PositionY(AnchorY yAnchor, float yPos, UiElement* element, UiScreen* parent);
}
