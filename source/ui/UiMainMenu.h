#pragma once
#include "UserInterface.h"
#include "MainMenuShared.h"

class UiMainMenu : public UiScreen
{
	MainMenuSharedResourcesPtr _sharedResources;

	GLfloat _elapsedTime;

public:
	explicit UiMainMenu(DivinitorApp* app);

	~UiMainMenu() override;
	void Init() override;
	void Draw(float deltaT) override;
	void Resize(int width, int height) override;
};
