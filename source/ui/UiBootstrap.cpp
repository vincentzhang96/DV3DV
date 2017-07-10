#include "../stdafx.h"
#include "UiBootstrap.h"
#include "uielement/UiLabel.h"

UiBootstrap::UiBootstrap(DivinitorApp* app) :
	UiScreen(app)
{
	_elapsedTime = 0;
	_sharedResources = std::make_shared<MainMenuSharedResources>(app->_textureManager);
}

UiBootstrap::~UiBootstrap()
{
}

void UiBootstrap::Init()
{
	_ui->InvalidateOldScreen();
	if (!_sharedResources->backgroundTex)
	{
		_sharedResources->backgroundTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000001)).second;
	}
	if (!_sharedResources->vignetteTex)
	{
		_sharedResources->vignetteTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000002)).second;
	}
	if (!_sharedResources->invVignetteTex)
	{
		_sharedResources->invVignetteTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000003)).second;
	}
	if (!_sharedResources->backgroundShaderProg)
	{
		_sharedResources->backgroundShaderProg = _app->_shaderManager->NewProgram();
		auto prog = _sharedResources->backgroundShaderProg;
		_app->_shaderManager->AttachAndCompileShader(prog, ppac::TPUID(0x0107, 0x0100, 0x00000002));	//	ui_adhoc_ortho_pt vertex shader
		_app->_shaderManager->AttachAndCompileShader(prog, ppac::TPUID(0x0108, 0x0101, 0x00000003));	//	ui_adhoc_tex_pt fragment shader
		if (!_app->_shaderManager->LinkAndFinishProgram(prog))
		{
			LOG(WARNING) << "Main menu background shader load failed";
			_app->_shaderManager->Unload(prog);
			_sharedResources->backgroundShaderProg = 0;
		}
	}
	if (!_sharedResources->ambienceSound)
	{
		_sharedResources->ambienceSound = _app->_audioManager->Load(ppac::TPUID(0x0302, 0x1001, 0x00000001));
		_app->_audioManager->Play(_sharedResources->ambienceSound);
	}
}



void UiBootstrap::Draw(float deltaT)
{
	_elapsedTime += deltaT;
	GLfloat pow = std::min(1.0F, _elapsedTime / 1.0F);
	GLfloat exp = std::max(0.0F, std::min(1.0F, (_elapsedTime - 0.5F) / 1.0F));
	_sharedResources->RenderBackground(this, exp, glm::fvec4(pow, pow, pow, 1.0F));

	DrawUiElements(deltaT);
}

void UiBootstrap::Resize(int width, int height)
{
	UiScreen::Resize(width, height);
	ClearUiElements();
	auto vahrLbl = new UiElements::StaticLabel(this, _app->fhGeomanistRegular, "VAHRHEDRAL", 18, dv3d::textOptionTracking(400));
	vahrLbl->_color = 0xFFA6AEB3;
	Position(UiElementAlignment::XLEFT, UiElementAlignment::YTOP, { 20, 20 }, vahrLbl, this);
	_elements.push_back(std::unique_ptr<UiElements::StaticLabel>(vahrLbl));

	auto divinitorLbl = new UiElements::StaticLabel(this, _app->fhGeomanistRegular, "DIVINITOR", 36, dv3d::textOptionTracking(300));
	divinitorLbl->_color = 0xFF98BCD4;
	Position(UiElementAlignment::XLEFT, UiElementAlignment::YTOP, { 20, 50 }, divinitorLbl, this);
	_elements.push_back(std::unique_ptr<UiElements::StaticLabel>(divinitorLbl));

	auto viewerLbl = new UiElements::StaticLabel(this, _app->fhGeomanistRegular, "3D VIEWER", 10, dv3d::textOptionTracking(200));
	viewerLbl->_color = 0xFF5AA9E5;
	Position(UiElementAlignment::XLEFT, UiElementAlignment::YTOP, { 20, 92 }, viewerLbl, this);
	_elements.push_back(std::unique_ptr<UiElements::StaticLabel>(viewerLbl));


	ProcessUiElements();
}
