#pragma once
#include "../ResourceManager.h"
#include "ShaderManager.h"

namespace dv3d
{
#define INVALID_FONTHANDLE 0

	typedef uint32_t FONTHANDLE;
	typedef uint32_t STATICTEXTHANDLE;
	typedef uint8_t FONTSIZE;
	typedef uint16_t FONTWEIGHT;

	struct Character;
	struct FontEntry;
	struct FontWeightEntry;
	struct FontWeightSizeEntry;

	struct FontWeightEntry
	{
		std::unordered_map<FONTWEIGHT, FontWeightSizeEntry> sizes;
		FontWeightEntry();
	};

	struct FontEntry
	{
		std::unordered_map<FONTSIZE, FontWeightEntry> weights;
		FT_Face zeroFace;
		std::vector<uint8_t> fontData;
		FontEntry();
	};

	struct StaticText
	{
		size_t textWidth;
		FONTHANDLE hFont;
		//	TODO
	};

	enum TEXTALIGNMENT
	{
		///	Text is aligned with the left edge at the text origin (default)
		TXTA_LEFT,
		///	Text is aligned with the text origin at the center
		TXTA_CENTER,
		///	Text is aligned with the right edge at the text origin
		TXTA_RIGHT
	};


	//	The first 128 ASCII characters are always prepopulated for every font+weight+size combination
	//	using a texture atlas
	//	For characters outside that range, we'll have individual textures since those tend to be sparse
	struct Character
	{
		///	The size of this character, in pixels. If this is an ASCII character,\
			calculate the ending UV coordinates by dividing by the atlas size and adding to asciiUVAtlasStart
		glm::ivec2 pxDimensions;
		/// The character's bearing, aka offset from baseline to the top left of the glyph
		glm::ivec2 pxBearing;
		///	Offset to advance to the next glyph
		GLuint pxAdvance;
		union
		{
			/// The UV coordinates where this character begins on the atlas (ASCII)
			glm::fvec2 asciiUVAtlasStart;
			///	The texture for this character (extended)
			GLuint extTexture;
		};

		Character();
	};

	struct FontWeightSizeEntry
	{
		GLuint asciiAtlasTex;
		Character asciiChars[128];
		std::unordered_map<uint32_t, Character> extChars;
		FontWeightSizeEntry();
	};

	class TextRenderer
	{
		packed_freelist<std::unique_ptr<FontEntry>> _fonts;
		resman::ResourceManager* _resManager;
		ShaderManager* _shdrManager;
		FT_Library ft;

		void InitFont(FontEntry &entry, FONTSIZE fontSize, FONTWEIGHT weight);
		void CreateAsciiAtlas(FontEntry &fontEntry, FontWeightSizeEntry &entry, FONTSIZE fontSize, FONTWEIGHT weight);
		void LoadGlyph(FontEntry &fontEntry, FontWeightSizeEntry &entry, FONTSIZE fontSize, FONTWEIGHT weight, uint32_t codepoint);
		static bool IsGlyphLoaded(FontEntry &fontEntry, FONTSIZE fontSize, FONTWEIGHT weight, uint32_t codepoint);
	public:
		explicit TextRenderer(resman::ResourceManager* resMan, ShaderManager* shdrManager);
		~TextRenderer();

		FONTHANDLE LoadFont(const resman::ResourceRequest &request);
		STATICTEXTHANDLE CreateStaticText(FONTHANDLE handle, const std::string &text, FONTSIZE fontSize, FONTWEIGHT weight = 0);
		void DrawStaticText2D(STATICTEXTHANDLE hStaticText, GLfloat x, GLfloat y, GLfloat z = 0, uint32_t color = 0xFFFFFFFF, TEXTALIGNMENT alignment = TXTA_LEFT);
		void DrawStaticText3D(STATICTEXTHANDLE hStaticText, glm::fmat4x4 projectionModelViewMatrix, uint32_t color = 0xFFFFFFFF, TEXTALIGNMENT alignment = TXTA_LEFT);
		void DrawDynamicText2D(FONTHANDLE hFont, const std::string &text, FONTSIZE fontSize, GLfloat x, GLfloat y, GLfloat z = 0, uint32_t color = 0xFFFFFFFF);
		void DrawDynamicText2D(FONTHANDLE hFont, const std::string &text, FONTSIZE fontSize, glm::fmat4x4 projectionModelViewMatrix, uint32_t color = 0xFFFFFFFF);
		size_t GetStaticTextWidth(STATICTEXTHANDLE hStaticText);
		size_t GetDynamicTextWidth(FONTHANDLE hFont, const std::string &text, FONTSIZE fontSize);
		void ReleaseStaticText(STATICTEXTHANDLE hStaticText);
		void UnloadFont(FONTHANDLE hFont);
	};

}


