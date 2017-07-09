#pragma once

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

	void RenderBackground(UiScreen* ui, GLfloat vigStr, glm::fvec4 color);
};

typedef std::shared_ptr<MainMenuSharedResources> MainMenuSharedResourcesPtr;