#pragma once

#include "Scene.h"
#include "3d/ShaderManager.h"
#include "3d/TextureManager.h"

class Renderer
{
private:
	int _fboWidth;
	int _fboHeight;
	int _winWidth;
	int _winHeight;
	float _supersamplingScale;

	GLuint finalOutputFbo;

	Scene* _currentScene;

public:

	Renderer();
	~Renderer();

	void Init(Scene* scene);
	void Resize(int width, int height);
	void Draw(float deltaTime);
};
