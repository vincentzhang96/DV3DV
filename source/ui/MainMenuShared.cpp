#include "../stdafx.h"
#include "../3d/TextureManager.h"
#include "../3d/TextRenderer.h"
#include "UserInterface.h"
#include "MainMenuShared.h"


MainMenuSharedResources::MainMenuSharedResources(dv3d::TextureManager* texMan) :
	_texMan(texMan)
{
	backgroundTex = 0;
	invVignetteTex = 0;
	vignetteTex = 0;
	backgroundShaderProg = 0;
	ambienceSound = 0;
}

MainMenuSharedResources::~MainMenuSharedResources()
{
	LOG(DEBUG) << "Unloading shared main menu resources";
	_texMan->Unload(backgroundTex);
	_texMan->Unload(invVignetteTex);
	_texMan->Unload(vignetteTex);
}

void MainMenuSharedResources::RenderBackground(UiScreen* ui, GLfloat vigStr, glm::fvec4 color)
{
	auto adHoc = &ui->_app->_adHocRenderer;
	auto screenSz = ui->_ui->GetScreenSize();
	GLfloat u0, u1, v0, v1;
	constexpr GLfloat texW = 1280;
	constexpr GLfloat texH = 768;
	constexpr GLfloat texW2H = texW / texH;
	auto scrW2H = screenSz.x / GLfloat(screenSz.y);
	u0 = 0;
	u1 = 1;
	v0 = 0;
	v1 = 1;
	GLfloat ratio = scrW2H / texW2H;
	if (ratio >= 1.0F)
	{
		//	Screen is wider aspect ratio than texture
		//	Make the texture taller/viewport shorter to compensate
		auto invRat = 1.0F - 1.0F / ratio;
		auto half = invRat / 2.0F;
		v0 = half;
		v1 = 1.0F - half;
	}
	else
	{
		//	Screen is narrow aspect ratio than texture
		//	Make the texture wider/viewport skinnier to compensate
		auto invRat = 1.0F - ratio;
		auto half = invRat / 2.0F;
		u0 = half;
		u1 = 1.0F - half;
	}
	auto prog = ui->_app->_shaderManager->Get(backgroundShaderProg);
	glUseProgram(prog);
	glUniformMatrix4fv(4, 1, GL_FALSE, ui->_ui->GetProjViewMatrixPtr());
	glActiveTexture(GL_TEXTURE0);
	glProgramUniform1i(prog, 5, 0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, backgroundTex);
	adHoc->BeginDraw(GL_TRIANGLE_FAN);
	adHoc->SetColorfv4(color);
	adHoc->AddVertexTexCoordf(0, 0, 0, u0, v1);
	adHoc->AddVertexTexCoordf(0, screenSz.y, 0, u0, v0);
	adHoc->AddVertexTexCoordf(screenSz.x, screenSz.y, 0, u1, v0);
	adHoc->AddVertexTexCoordf(screenSz.x, 0, 0, u1, v1);
	adHoc->EndDraw();

	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glBindTexture(GL_TEXTURE_2D, vignetteTex);
	adHoc->BeginDraw(GL_TRIANGLE_FAN);
	adHoc->SetColorf(1.0, 1.0, 1.0, vigStr);
	adHoc->AddVertexTexCoordf(0, 0, 0, 0, 1);
	adHoc->AddVertexTexCoordf(0, screenSz.y, 0, 0, 0);
	adHoc->AddVertexTexCoordf(screenSz.x, screenSz.y, 0, 1, 0);
	adHoc->AddVertexTexCoordf(screenSz.x, 0, 0, 1, 1);
	adHoc->EndDraw();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(0);

//	std::stringstream fmt;
//	fmt.precision(2);
//	fmt.setf(std::ios::fixed, std::ios::floatfield);
//	fmt << "scrW2H " << scrW2H << ", texW2H " << texW2H << ", ratio " << ratio;
//	glm::fvec2 pos = Position(UiElementAlignment::XLEFT, UiElementAlignment::YTOP, { 10, 200 }, { 0, 0 }, screenSz);
//	ui->_text->DrawDynamicText2D(ui->_app->fhLatoRegular, fmt.str(), 18, pos.x, pos.y, 0, 0xFF5AA9E5, dv3d::textOptionAlignment(dv3d::TXTA_LEFT));
}