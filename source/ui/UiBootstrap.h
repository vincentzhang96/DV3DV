#pragma once
#include "../UserInterface.h"

struct MainMenuSharedResources
{
	dv3d::GLTEXHANDLE backgroundTex;
	dv3d::GLTEXHANDLE vignetteTex;
	dv3d::GLTEXHANDLE invVignetteTex;
};

class UiBootstrap : public UIScreen
{
public:
	explicit UiBootstrap(DivinitorApp* app);

	~UiBootstrap() override;
	void Init() override;
	void Draw(float deltaT) override;
};

