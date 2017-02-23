#pragma once
#include "../UserInterface.h"

class MainMenuSharedResources
{
	dv3d::TextureManager* _texMan;

public:
	dv3d::GLTEXHANDLE backgroundTex;
	dv3d::GLTEXHANDLE vignetteTex;
	dv3d::GLTEXHANDLE invVignetteTex;

	explicit MainMenuSharedResources(dv3d::TextureManager* texMan);
	~MainMenuSharedResources();
};

class UiBootstrap : public UIScreen
{
	typedef std::shared_ptr<MainMenuSharedResources> MainMenuSharedResourcesPtr;
	MainMenuSharedResourcesPtr _sharedResources;

public:
	explicit UiBootstrap(DivinitorApp* app);

	~UiBootstrap() override;
	void Init() override;
	void Draw(float deltaT) override;
};

