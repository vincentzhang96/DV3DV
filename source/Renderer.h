#pragma once

#include "Scene.h"

class Renderer
{
private:

public:
	Renderer();
	~Renderer();

	void Init(Scene* scene);
	void Resize(int width, int height);
	void Draw(float deltaTime);
};
