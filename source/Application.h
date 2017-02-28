#pragma once
#include "ResourceManager.h"
#include "Renderer.h"
#include "3d/TextRenderer.h"
#include "ui/AdHocRenderer.h"

class UserInterface;

class DivinitorApp
{
	resman::ResourceManager* _resMan;

	Scene* _scene;

	uint64_t _lastFrameTimeNs;

	inline float UpdateTime();

	uint64_t _lastFPSUpdateTime;
	uint32_t _fpsFrameCounter;
	uint32_t _lastFps;
	float _lastFrameDrawTimeMs;

	int viewportWidth;
	int viewportHeight;

	bool _displayDebug;
	
	static inline uint64_t GetSystemTimeNanos();

public:
	Renderer* _renderer;
	dv3d::adhoc::Renderer _adHocRenderer;
	dv3d::ShaderManager* _shaderManager;
	dv3d::TextureManager* _textureManager;
	dv3d::TextRenderer* _textRenderer;
	UserInterface* _userInterface;

	glm::ivec2 _mouseCoords;

	//	TODO factor this out into some sort of fontface manager
	dv3d::FONTHANDLE fhLatoRegular;
	dv3d::FONTHANDLE fhGeomanistRegular;
	dv3d::FONTHANDLE fhNanumSemibold;
	dv3d::FONTHANDLE fhJunProRegular;

	DivinitorApp(resman::ResourceManager* resman);
	~DivinitorApp();

	void FirstFrameInit();

	void Draw();

	void OnViewportResized(int width, int height);

	void OnKeyPressed(int keyCode);

	void OnKeyReleased(int keyCode);

	void OnMouseMoved(int x, int y, int buttonCode);

	void OnMouseButtonPressed(int x, int y, int buttonCode);

	void OnMouseButtonReleased(int x, int y, int buttonCode);

	
};
