#pragma once

class DivinitorApp;

class UserInterface
{
	DivinitorApp* _app;

public:
	explicit UserInterface(DivinitorApp* app);
	~UserInterface();

	void Draw(float deltaTime);

	void Resize(int width, int height);
};
