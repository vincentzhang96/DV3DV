#include "stdafx.h"
#include "Application.h"
#include "UserInterface.h"

float DivinitorApp::UpdateTime()
{
	uint64_t deltaNs = 0;
	uint64_t nowNs;
	nowNs = GetSystemTimeNanos();
	deltaNs = nowNs - _lastFrameTimeNs;
	//	Calculate deltaT
	_lastFrameTimeNs = nowNs;
	uint64_t timeSinceFPSUpdate = nowNs - _lastFPSUpdateTime;
	++_fpsFrameCounter;
	if (timeSinceFPSUpdate >= 1e9)
	{
		_lastFrameDrawTimeMs = timeSinceFPSUpdate / _fpsFrameCounter / 1e6F;
		_lastFps = float(_fpsFrameCounter) / (timeSinceFPSUpdate / 1e9F);
		_fpsFrameCounter = 0;
		_lastFPSUpdateTime = nowNs;
	}
	return deltaNs / 1e9F;
}

uint64_t DivinitorApp::GetSystemTimeNanos()
{
	SYSTEMTIME currTime;
	GetSystemTime(&currTime);
	FILETIME currFTime;
	SystemTimeToFileTime(&currTime, &currFTime);
	ULARGE_INTEGER uTime;
	uTime.HighPart = currFTime.dwHighDateTime;
	uTime.LowPart = currFTime.dwLowDateTime;
	return uTime.QuadPart * 100;
}

DivinitorApp::DivinitorApp(resman::ResourceManager* resman) : 
_adHocRenderer(0xFFFF)
{
	_resMan = resman;
	_shaderManager = new dv3d::ShaderManager(_resMan);
	_textureManager = new dv3d::TextureManager(_resMan);
	_renderer = new Renderer();
	_textRenderer = new dv3d::TextRenderer(_resMan, _shaderManager);
	_scene = nullptr;
	_userInterface = new UserInterface(this);
	_lastFrameTimeNs = 0;
	_lastFPSUpdateTime = 0;

}

DivinitorApp::~DivinitorApp()
{
	delete _userInterface;
	delete _textRenderer;
	delete _textureManager;
	delete _shaderManager;
}

void DivinitorApp::FirstFrameInit()
{
	_adHocRenderer.PostRendererInit();
	_textRenderer->PostRendererInit();
	//	Load fonts
	fhLatoRegular = _textRenderer->LoadFont(ppac::TPUID(0x0500, 0x0001, 0x00000100));
	fhGeomanistRegular = _textRenderer->LoadFont(ppac::TPUID(0x0501, 0x0001, 0x00000200));
	fhNanumSemibold = _textRenderer->LoadFont(ppac::TPUID(0x0500, 0x0001, 0x00000300));
	fhJunProRegular = _textRenderer->LoadFont(ppac::TPUID(0x0501, 0x0001, 0x00000400));


	//	Fudge our time
	UpdateTime();

	//	Init UI
	_userInterface->Init();
}

void DivinitorApp::Draw()
{
	float deltaT = UpdateTime();
	if (_scene)
	{
		_renderer->Draw(deltaT);
	}
	_userInterface->Draw(deltaT);

	std::stringstream fmt;
	fmt.precision(2);
	fmt.setf(std::ios::fixed, std::ios::floatfield);
	fmt << _lastFps << " FPS, " << _lastFrameDrawTimeMs << "ms/frame\n";
	fmt << "AHR " << _adHocRenderer._drawCalls << " calls, " << _adHocRenderer._totalPolysDrawnThisFrame << " polys\n";
	auto txtStats = _textRenderer->_statistics;
	fmt << "TXT " << txtStats.dynamicTextsDrawn << " dynamic, " <<  txtStats.staticTextsDrawn << " static, " << txtStats.extGlyphsDrawn << " EXT, " << txtStats.asciiBatchesDrawn << " ABTCH\n";
	_textRenderer->DrawDynamicText2D(fhLatoRegular, fmt.str(), 18, viewportWidth - 10.0F, viewportHeight - 20.0F, 0, 0xFF0088FF, dv3d::textOptionAlignment(dv3d::TXTA_RIGHT));
	_adHocRenderer.FinishFrame();
	_textRenderer->FinishFrame();
}

void DivinitorApp::OnViewportResized(int width, int height)
{
	viewportWidth = width;
	viewportHeight = height;
	_textRenderer->UpdateScreenSize(width, height);
	_userInterface->Resize(width, height);
}