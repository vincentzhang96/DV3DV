#include "../stdafx.h"
#include "UiBootstrap.h"


UiBootstrap::UiBootstrap(DivinitorApp* app) :
	UIScreen(app)
{;
}

UiBootstrap::~UiBootstrap()
{
}

void UiBootstrap::Draw(float deltaT)
{



	_text->DrawDynamicText2D(
		_app->fhGeomanistRegular,
		"DIVINITOR",
		36,
		10,
		_ui->GetScreenSize().y - 46,
		0,
		0xFF98BCD4,
		dv3d::textOptionTracking(300)
	);
}
