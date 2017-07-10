#pragma once
#include "ResourceManager.h"
#include "3d/Renderer.h"
#include "3d/TextRenderer.h"
#include "ui/AdHocRenderer.h"
#include "audio/AudioManager.h"

class UserInterface;

const int DV_TPS = 20;
#define DV_TICK_INTERVAL_NS (1e9 / DV_TPS)

class DivinitorApp
{
	resman::ResourceManager* _resMan;

	Scene* _scene;

	uint64_t _lastFrameTimeNs;

	inline float UpdateFrameTime(uint64_t* nowNsOut = nullptr);

	uint64_t _lastFPSUpdateTime;
	uint32_t _fpsFrameCounter;
	uint32_t _lastFps;
	float _lastFrameDrawTimeMs;
	uint64_t _frameDrawTimeAccum;

	int viewportWidth;
	int viewportHeight;

	bool _displayDebug;
	
	static inline uint64_t GetSystemTimeNanos();

	inline float UpdateTickTime(uint64_t* nowNsOut = nullptr);

	uint64_t _lastSimTickTimeNs;
	uint32_t _tpsCounter;
	uint32_t _lastTps;
	uint64_t _lastTPSUpdateTime;
	float _lastTickSimTimeMs;
	uint64_t _tickSimTimeAccum;
	bool _shouldTick;

public:
	Renderer* _renderer;
	dv3d::adhoc::Renderer _adHocRenderer;
	dv3d::ShaderManager* _shaderManager;
	dv3d::TextureManager* _textureManager;
	dv3d::TextRenderer* _textRenderer;
	UserInterface* _userInterface;
	AudioManager* _audioManager;

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

	void Tick();

	void OnViewportResized(int width, int height);

	void OnKeyPressed(int keyCode);

	void OnKeyReleased(int keyCode);

	void OnMouseMoved(int x, int y, int buttonCode);

	void OnMouseButtonPressed(int x, int y, int buttonCode);

	void OnMouseButtonReleased(int x, int y, int buttonCode);

};
