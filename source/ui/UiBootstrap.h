#pragma once
#include "../UserInterface.h"
#include "MainMenuShared.h"

class UiBootstrap : public UiScreen
{
	typedef std::shared_ptr<MainMenuSharedResources> MainMenuSharedResourcesPtr;
	MainMenuSharedResourcesPtr _sharedResources;

	GLfloat _elapsedTime;

public:
	explicit UiBootstrap(DivinitorApp* app);

	~UiBootstrap() override;
	void Init() override;
	void Draw(float deltaT) override;
	void Resize(int width, int height) override;
};

