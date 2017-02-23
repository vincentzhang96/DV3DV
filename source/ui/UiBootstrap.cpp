#include "../stdafx.h"
#include "UiBootstrap.h"


MainMenuSharedResources::MainMenuSharedResources(dv3d::TextureManager* texMan) :
_texMan(texMan)
{
	backgroundTex = 0;
	invVignetteTex = 0;
	vignetteTex = 0;
}

MainMenuSharedResources::~MainMenuSharedResources()
{
	LOG(DEBUG) << "Unloading shared main menu resources";
	_texMan->Unload(backgroundTex);
	_texMan->Unload(invVignetteTex);
	_texMan->Unload(vignetteTex);
}

UiBootstrap::UiBootstrap(DivinitorApp* app) :
	UIScreen(app)
{
	_sharedResources = std::make_shared<MainMenuSharedResources>(app->_textureManager);
}

UiBootstrap::~UiBootstrap()
{
}

void UiBootstrap::Init()
{
	_ui->InvalidateOldScreen();
	_sharedResources->backgroundTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000001)).second;
	_sharedResources->vignetteTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000002)).second;
	_sharedResources->invVignetteTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000003)).second;
}

void UiBootstrap::Draw(float deltaT)
{



	_text->DrawDynamicText2D(
		_app->fhGeomanistRegular,
		"TEST",
		36,
		10,
		_ui->GetScreenSize().y - 46,
		0,
		0xFF98BCD4,
		dv3d::textOptionTracking(300)
	);
}
