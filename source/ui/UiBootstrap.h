#pragma once
#include "../UserInterface.h"

class UiBootstrap : public UIScreen
{
public:
	explicit UiBootstrap(DivinitorApp* app);

	~UiBootstrap() override;
	void Draw(float deltaT) override;
};

