#pragma once
#include "../ResourceManager.h"

namespace dv3d
{

	struct FontEntry
	{
		
	};

	class TextRenderer
	{
		typedef uint32_t FONTHANDLE;
		typedef uint32_t STATICTEXTHANDLE;
	public:
		TextRenderer();
		~TextRenderer();

		FONTHANDLE LoadFont(const resman::ResourceRequest &request);
		STATICTEXTHANDLE CreateStaticText(const FONTHANDLE &handle, const std::string &text);
		void DrawStaticText(const STATICTEXTHANDLE &textHandle, GLfloat x, GLfloat y, GLfloat z, uint32_t color);
		void DrawDynamicText(const FONTHANDLE &handle, const std::string &text, size_t fontSize, GLfloat x, GLfloat y, GLfloat z, uint32_t color);
	};

}


