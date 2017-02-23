#pragma once
#include "../UserInterface.h"

class MainMenuSharedResources
{
	dv3d::TextureManager* _texMan;

public:
	dv3d::GLTEXHANDLE backgroundTex;
	dv3d::GLTEXHANDLE vignetteTex;
	dv3d::GLTEXHANDLE invVignetteTex;
	dv3d::GLPROGHANDLE backgroundShaderProg;

	explicit MainMenuSharedResources(dv3d::TextureManager* texMan);
	~MainMenuSharedResources();

	void RenderBackground(UIScreen* ui, GLfloat vigStr, glm::fvec4 color);
};

class UiBootstrap : public UIScreen
{
	typedef std::shared_ptr<MainMenuSharedResources> MainMenuSharedResourcesPtr;
	MainMenuSharedResourcesPtr _sharedResources;

	GLfloat _elapsedTime;

public:
	explicit UiBootstrap(DivinitorApp* app);

	~UiBootstrap() override;
	void Init() override;
	void Draw(float deltaT) override;
};

