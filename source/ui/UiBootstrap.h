#pragma once
#include "../Application.h"
#include "../UserInterface.h"
#include "../3d/TextRenderer.h"
#include "UIScreen.h"

class UiBootstrap : public UIScreen
{
	DivinitorApp* _app;
	UserInterface* _ui;
	dv3d::TextRenderer* _text;
public:
	explicit UiBootstrap(DivinitorApp* app);

	~UiBootstrap() override;
	void Draw(float deltaT) override;
};

