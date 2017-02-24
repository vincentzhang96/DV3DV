#include "../stdafx.h"
#include "UiBootstrap.h"
#include "uielement/UiLabel.h"


MainMenuSharedResources::MainMenuSharedResources(dv3d::TextureManager* texMan) :
_texMan(texMan)
{
	backgroundTex = 0;
	invVignetteTex = 0;
	vignetteTex = 0;
	backgroundShaderProg = 0;
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
	bool widthIsMax = screenSz.x > screenSz.y;
	GLfloat u0, u1, v0, v1;
	constexpr GLfloat texW = 1280;
	constexpr GLfloat texH = 768;
	if (widthIsMax)
	{
		u0 = 0;
		u1 = 1;
		GLfloat xRescale = screenSz.x / texW;
		GLfloat yRescaled = screenSz.y / xRescale;
		GLfloat yPartial = texH / yRescaled - 1.0F;
		GLfloat halfPartial = yPartial / 2.0F;
		v0 = halfPartial;
		v1 = 1.0 - halfPartial;
	}
	else
	{
		v0 = 0;
		v1 = 1;
		GLfloat yRescale = screenSz.y / texH;
		GLfloat xRescaled = screenSz.x / yRescale;
		GLfloat xPartial = texW / xRescaled - 1.0F;
		GLfloat halfPartial = xPartial / 2.0F;
		u0 = halfPartial;
		u1 = 1.0 - halfPartial;
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
}


UiBootstrap::UiBootstrap(DivinitorApp* app) :
	UiScreen(app)
{
	_elapsedTime = 0;
	_sharedResources = std::make_shared<MainMenuSharedResources>(app->_textureManager);
}

UiBootstrap::~UiBootstrap()
{
}

void UiBootstrap::Init()
{
	_ui->InvalidateOldScreen();
	if (!_sharedResources->backgroundTex)
	{
		_sharedResources->backgroundTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000001)).second;
	}
	if (!_sharedResources->vignetteTex)
	{
		_sharedResources->vignetteTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000002)).second;
	}
	if (!_sharedResources->invVignetteTex)
	{
		_sharedResources->invVignetteTex = _app->_textureManager->LoadAndGet(ppac::TPUID(0x0206, 0x1001, 0x00000003)).second;
	}
	if (!_sharedResources->backgroundShaderProg)
	{
		_sharedResources->backgroundShaderProg = _app->_shaderManager->NewProgram();
		auto prog = _sharedResources->backgroundShaderProg;
		_app->_shaderManager->AttachAndCompileShader(prog, ppac::TPUID(0x0107, 0x0100, 0x00000002));	//	ui_adhoc_ortho_pt vertex shader
		_app->_shaderManager->AttachAndCompileShader(prog, ppac::TPUID(0x0108, 0x0101, 0x00000003));	//	ui_adhoc_tex_pt fragment shader
		if (!_app->_shaderManager->LinkAndFinishProgram(prog))
		{
			LOG(WARNING) << "Main menu background shader load failed";
			_app->_shaderManager->Unload(prog);
			_sharedResources->backgroundShaderProg = 0;
		}
	}
}



void UiBootstrap::Draw(float deltaT)
{
	_elapsedTime += deltaT;
	GLfloat pow = std::min(1.0F, _elapsedTime / 1.0F);
	GLfloat exp = std::max(0.0F, std::min(1.0F, (_elapsedTime - 0.5F) / 1.0F));
	_sharedResources->RenderBackground(this, exp, glm::fvec4(pow, pow, pow, 1.0F));

	DrawUiElements(deltaT);
}

void UiBootstrap::Resize(int width, int height)
{
	UiScreen::Resize(width, height);
	_elements.clear();
	auto vahrLbl = new UiElements::StaticLabel(this, _app->fhGeomanistRegular, "VAHRHEDRAL", 18, dv3d::textOptionTracking(400));
	vahrLbl->_color = 0xFFA6AEB3;
	Position(UiElementAlignment::XLEFT, UiElementAlignment::YTOP, { 20, 20 }, vahrLbl, this);
	_elements.push_back(std::unique_ptr<UiElements::StaticLabel>(vahrLbl));

	auto divinitorLbl = new UiElements::StaticLabel(this, _app->fhGeomanistRegular, "DIVINITOR", 36, dv3d::textOptionTracking(300));
	divinitorLbl->_color = 0xFF98BCD4;
	Position(UiElementAlignment::XLEFT, UiElementAlignment::YTOP, { 20, 50 }, divinitorLbl, this);
	_elements.push_back(std::unique_ptr<UiElements::StaticLabel>(divinitorLbl));

	auto viewerLbl = new UiElements::StaticLabel(this, _app->fhGeomanistRegular, "3D VIEWER", 10, dv3d::textOptionTracking(200));
	viewerLbl->_color = 0xFF5AA9E5;
	Position(UiElementAlignment::XLEFT, UiElementAlignment::YTOP, { 20, 92 }, viewerLbl, this);
	_elements.push_back(std::unique_ptr<UiElements::StaticLabel>(viewerLbl));


	ProcessUiElements();
}
