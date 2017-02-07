#include "stdafx.h"
#include "Renderer.h"

Renderer::Renderer()
{

}

Renderer::~Renderer()
{
}

void Renderer::Init(Scene* scene)
{
}

void Renderer::Resize(int width, int height)
{
	_winWidth = width;
	_winHeight = height;
	_fboWidth = _winWidth * _supersamplingScale;
	_fboHeight = _winHeight * _supersamplingScale;

	glFinish();

	//	Rebuild FBOs



}

void Renderer::Draw(float deltaTime)
{
}
