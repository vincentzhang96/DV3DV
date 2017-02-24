#pragma once
#include "../../3d/TextRenderer.h"
#include "../../UserInterface.h"

namespace UiElements
{
	class StaticLabel : public UiElement
	{
		dv3d::STATICTEXTHANDLE _handle;
		glm::fvec2 _textOffset;
		bool _takeOwnership;

		void _ProcessText();
	public:
		uint32_t _color;
		explicit StaticLabel(UiScreen* parent, dv3d::STATICTEXTHANDLE handle);
		explicit StaticLabel(UiScreen* parent, dv3d::FONTHANDLE handle, const std::string &text, dv3d::FONTSIZE fontSize, dv3d::TextOptions options = 0);
		~StaticLabel();
		void Draw(float deltaT) override;
	};
}
