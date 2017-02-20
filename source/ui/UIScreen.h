#pragma once

class UIScreen
{

public:
	virtual ~UIScreen() {}

	virtual void Draw(float deltaT) = 0;
};
