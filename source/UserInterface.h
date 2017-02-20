#pragma once
#include "3d/ShaderManager.h"

class DivinitorApp;

class UserInterface
{
	DivinitorApp* _app;

	GLuint _screenFbo;
	GLuint _screenFboColorTex;
	GLuint _screenRbo;
	GLuint _screenQuadVao;

	dv3d::GLPROGHANDLE _screenFboProg;

	glm::ivec2 _size;

public:
	explicit UserInterface(DivinitorApp* app);
	~UserInterface();

	void Init();

	void Draw(float deltaTime);

	void Resize(int width, int height);
};
