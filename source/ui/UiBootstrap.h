#pragma once
#include "UserInterface.h"
#include "MainMenuShared.h"

class UiBootstrap : public UiScreen
{
	MainMenuSharedResourcesPtr _sharedResources;

	GLfloat _elapsedTime;

public:
	explicit UiBootstrap(DivinitorApp* app);

	~UiBootstrap() override;
	void Init() override;
	void Draw(float deltaT) override;
	void Resize(int width, int height) override;
};

