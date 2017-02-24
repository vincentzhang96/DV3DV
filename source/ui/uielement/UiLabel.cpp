#include "../stdafx.h"
#include "UiLabel.h"

void UiElements::StaticLabel::_ProcessText()
{
	_size.x = _ui->_text->GetStaticTextWidth(_handle);
	_size.y = _ui->_text->GetStaticTextHeight(_handle);
	_color = 0xFFFFFFFF;
	auto opt = _ui->_text->GetStaticTextOptions(_handle);
	_textOffset = { 0, _ui->_text->GetStaticTextSize(_handle) / 4.0F };
	if (opt.flags & TEXTOPTION_ALIGNMENT)
	{
		switch (opt.alignment)
		{
		case dv3d::TXTA_CENTER:
			_textOffset.x = _size.x / 2.0F;
			break;
		case dv3d::TXTA_RIGHT:
			_textOffset.x = _size.x;
			break;
		default:
			break;
		}
	}
	_textOffset.x = std::round(_textOffset.x);
	_textOffset.y = std::round(_textOffset.y);
}

UiElements::StaticLabel::StaticLabel(UiScreen* parent, dv3d::STATICTEXTHANDLE handle) :
UiElement(parent), _handle(handle)
{
	_takeOwnership = false;
	_ProcessText();
}

UiElements::StaticLabel::StaticLabel(UiScreen* parent, dv3d::FONTHANDLE handle, const std::string& text, dv3d::FONTSIZE fontSize, dv3d::TextOptions options) :
UiElement(parent)
{
	_takeOwnership = true;
	_handle = parent->_text->CreateStaticText(handle, text, fontSize, options);
	_ProcessText();
}

UiElements::StaticLabel::~StaticLabel()
{
	if (_takeOwnership)
	{
		_ui->_text->ReleaseStaticText(_handle);
	}
}

void UiElements::StaticLabel::Draw(float deltaT)
{
	_ui->_text->DrawStaticText2D(_handle, _pos.x + _textOffset.x, _pos.y + _textOffset.y, 0, _color);
}
