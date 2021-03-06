#include "stdafx.h"
#include "Application.h"
#include "ui/UserInterface.h"
#include "ui/UiBootstrap.h"
#include "main.h"

float DivinitorApp::UpdateFrameTime(uint64_t* nowNsOut)
{
	uint64_t deltaNs;
	uint64_t nowNs;
	nowNs = GetSystemTimeNanos();
	deltaNs = nowNs - _lastFrameTimeNs;
	//	Calculate deltaT
	_lastFrameTimeNs = nowNs;
	auto timeSinceFPSUpdate = nowNs - _lastFPSUpdateTime;
	++_fpsFrameCounter;
	if (timeSinceFPSUpdate >= 1e9)
	{
		_lastFrameDrawTimeMs = _frameDrawTimeAccum / _fpsFrameCounter / 1e6F;
		_lastFps = float(_fpsFrameCounter) / (timeSinceFPSUpdate / 1e9F);
		_fpsFrameCounter = 0;
		_frameDrawTimeAccum = 0;
		_lastFPSUpdateTime = nowNs;
	}
	if (nowNsOut)
	{
		*nowNsOut = nowNs;
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

float DivinitorApp::UpdateTickTime(uint64_t* nowNsOut)
{
	uint64_t deltaNs;
	uint64_t nowNs;
	nowNs = GetSystemTimeNanos();
	deltaNs = nowNs - _lastSimTickTimeNs;
	if (deltaNs >= DV_TICK_INTERVAL_NS)
	{
		_shouldTick = true;
		_lastSimTickTimeNs = nowNs;
		++_tpsCounter;
		auto timeSinceTPSUpdate = nowNs - _lastTPSUpdateTime;
		if (timeSinceTPSUpdate >= 1e9)
		{
			_lastTickSimTimeMs = _tickSimTimeAccum / _tpsCounter / 1e6F;
			_lastTps = float(_tpsCounter) / (timeSinceTPSUpdate / 1e9F);
			_tpsCounter = 0;
			_tickSimTimeAccum = 0;
			_lastTPSUpdateTime = nowNs;
		}
		if (nowNsOut)
		{
			*nowNsOut = nowNs;
		}
		return deltaNs / 1e9F;
	}
	else
	{
		_shouldTick = false;
		return 0;
	}
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
	_audioManager = new AudioManager(_resMan);
	_lastFrameTimeNs = 0;
	_lastFPSUpdateTime = 0;
	_frameDrawTimeAccum = 0;
	_lastSimTickTimeNs = 0;
	_lastTPSUpdateTime = 0;
	_tickSimTimeAccum = 0;
	_displayDebug = false;
	_mouseCoords = { 0, 0 };
}

DivinitorApp::~DivinitorApp()
{
	delete _audioManager;
	delete _userInterface;
	delete _textRenderer;
	delete _textureManager;
	delete _shaderManager;
}

void DivinitorApp::FirstFrameInit()
{
	wglSwapIntervalEXT(config.vsync);
	_adHocRenderer.PostRendererInit();
	_textRenderer->PostRendererInit();
	//	Load fonts
	fhLatoRegular = _textRenderer->LoadFont(ppac::TPUID(0x0500, 0x0001, 0x00000100));
	fhGeomanistRegular = _textRenderer->LoadFont(ppac::TPUID(0x0501, 0x0001, 0x00000200));
	fhNanumSemibold = _textRenderer->LoadFont(ppac::TPUID(0x0500, 0x0001, 0x00000300));
	fhJunProRegular = _textRenderer->LoadFont(ppac::TPUID(0x0501, 0x0001, 0x00000400));

	_audioManager->Init();

	//	Fudge our time
	UpdateFrameTime();
	UpdateTickTime();

	//	Init UI
	_userInterface->Init();
}

void DivinitorApp::Draw()
{
	uint64_t nowNs;
	float deltaT = UpdateFrameTime(&nowNs);
	if (_scene)
	{
		_renderer->Draw(deltaT);
	}
	_userInterface->Draw(deltaT);
	_frameDrawTimeAccum += GetSystemTimeNanos() - nowNs;

	Tick();

	if (_displayDebug)
	{
		PROCESS_MEMORY_COUNTERS memCounters;
		GetProcessMemoryInfo(GetCurrentProcess(), &memCounters, sizeof(PPROCESS_MEMORY_COUNTERS));

		std::stringstream fmt;
		fmt.precision(2);
		fmt.setf(std::ios::fixed, std::ios::floatfield);
		fmt << "Working " << memCounters.WorkingSetSize / 1024.0 / 1024.0 << " MB\n";
		fmt << "VP " << viewportWidth << "x" << viewportHeight << "\n";
		fmt << _lastFps << " FPS, " << _lastFrameDrawTimeMs << "ms/frame";
		if (config.vsync)
		{
			fmt << " VSYNC";
		}
		fmt << "\n";
		fmt << _lastTps << " TPS, " << _lastTickSimTimeMs << "ms/tick\n";
		fmt << "AHR " << _adHocRenderer._drawCalls << " calls, " << _adHocRenderer._totalPolysDrawnThisFrame << " polys\n";
		auto txtStats = _textRenderer->_statistics;
		fmt << "TXT " << txtStats.dynamicTextsDrawn << " dynamic, " << txtStats.staticTextsDrawn << " static, " << txtStats.extGlyphsDrawn << " EXT, " << txtStats.asciiBatchesDrawn << " ABTCH\n";
		fmt << "MOUSE (" << _mouseCoords.x << ", " << _mouseCoords.y << ")\n";

		glm::fvec2 pos = Position(UiElementAlignment::XRIGHT, UiElementAlignment::YTOP, { 10, 20 }, { 0, 0 }, { viewportWidth, viewportHeight });
		_textRenderer->DrawDynamicText2D(fhLatoRegular, fmt.str(), 18, pos.x, pos.y, 0, 0xFF5AA9E5, dv3d::textOptionAlignment(dv3d::TXTA_RIGHT));
	}
	_adHocRenderer.FinishFrame();
	_textRenderer->FinishFrame();
}

void DivinitorApp::Tick()
{
	uint64_t nowNs;
	auto deltaT = UpdateTickTime(&nowNs);
	if (!_shouldTick)
	{
		return;
	}
	_shouldTick = false;


	_tickSimTimeAccum = GetSystemTimeNanos() - nowNs;
}

void DivinitorApp::OnViewportResized(int width, int height)
{
	viewportWidth = width;
	viewportHeight = height;
	_textRenderer->UpdateScreenSize(width, height);
	_userInterface->Resize(width, height);
}

void DivinitorApp::OnKeyPressed(int keyCode)
{
	if (!_userInterface->HandleKeyPress(keyCode)) {

	}
}

void DivinitorApp::OnKeyReleased(int keyCode)
{
	if (!_userInterface->HandleKeyRelease(keyCode)) {
		//	Debug hook
		if (keyCode == 'D')
		{
			_displayDebug = !_displayDebug;
			return;
		}
		if (keyCode == 'V')
		{
			config.vsync = !config.vsync;
			wglSwapIntervalEXT(config.vsync);
			_WriteConfig(config);
			return;
		}
	}
}

void DivinitorApp::OnMouseMoved(int x, int y, int buttonCode)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > viewportWidth) x = viewportWidth;
	if (y > viewportHeight) y = viewportHeight;
	_mouseCoords.x = x;
	_mouseCoords.y = y;
}

void DivinitorApp::OnMouseButtonPressed(int x, int y, int buttonCode)
{
	if (buttonCode & MK_LBUTTON)
	{
		//	Left mouse
		if (!_userInterface->HandleMouseClick(x, y, MK_LBUTTON))
		{
			//	Not handled by UI, propogate to underlying layers

		}
	}
	if (buttonCode & MK_RBUTTON)
	{
		//	Right mouse
		if (!_userInterface->HandleMouseClick(x, y, MK_RBUTTON))
		{
			//	Not handled by UI, propogate to underlying layers

		}
	}
}

void DivinitorApp::OnMouseButtonReleased(int x, int y, int buttonCode)
{
	if (buttonCode & MK_LBUTTON)
	{
		//	Left mouse
		if (!_userInterface->HandleMouseRelease(x, y, MK_LBUTTON))
		{
			//	Not handled by UI, propogate to underlying layers

		}
	}
	if (buttonCode & MK_RBUTTON)
	{
		//	Right mouse
		if (!_userInterface->HandleMouseRelease(x, y, MK_RBUTTON))
		{
			//	Not handled by UI, propogate to underlying layers

		}
	}
}
