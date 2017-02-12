#pragma once
#include "../ResourceManager.h"
#include "ShaderManager.h"

namespace dv3d
{
#define INVALID_FONTHANDLE 0

	typedef uint32_t FONTHANDLE;
	typedef uint32_t STATICTEXTHANDLE;
	typedef uint8_t FONTSIZE;

	struct Character;
	struct FontEntry;
	struct FontSizeEntry;

	struct FontEntry
	{
		std::unordered_map<FONTSIZE, FontSizeEntry> sizes;
		FT_Face face;
		std::vector<uint8_t> fontData;
		FontEntry();
		~FontEntry();
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
		///	The size of this character, in pixels. If this is an ASCII character,
		/// calculate the ending UV coordinates by dividing by the atlas size and adding to asciiUVAtlasStart
		///	If the x dimension is 0 then this character is empty and no drawcall should be made.
		glm::ivec2 pxDimensions;
		/// The character's bearing, aka offset from baseline to the top left of the glyph
		glm::ivec2 pxBearing;
		///	Offset to advance to the next glyph
		GLuint pxAdvance;
		union
		{
			/// The UV coordinates where this character begins on the atlas (ASCII)
			glm::fvec2 asciiUVAtlasStart;
			///	The texture for this character (extended). If 0, then the character is blank (match with pxDimensions).
			GLuint extTexture;
		};

		Character();
	};

	struct FontSizeEntry
	{
		GLuint asciiAtlasTex;
		Character asciiChars[128];
		std::unordered_map<uint32_t, Character> extChars;
		FT_Size ftSize;
		FontSizeEntry();
		~FontSizeEntry();
	};

	class TextRenderer
	{
		packed_freelist<std::unique_ptr<FontEntry>> _fonts;
		resman::ResourceManager* _resManager;
		ShaderManager* _shdrManager;
		FT_Library ft;
		int screenWidth;
		int screenHeight;
		glm::fmat4 projView;
		GLuint quadVertexArray = 0;
		GLuint quadVertexBuffer = 0;
		GLPROGHANDLE h2dTextShader = 0;


		void InitFont(FontEntry* entry, FONTSIZE fontSize);
		void CreateAsciiAtlas(FontEntry* fontEntry, FontSizeEntry* entry, FONTSIZE fontSize);
		void LoadExtGlyph(FontEntry* fontEntry, FontSizeEntry* entry, FONTSIZE fontSize, uint32_t codepoint);
		static bool IsGlyphLoaded(FontEntry* fontEntry, FONTSIZE fontSize, uint32_t codepoint);

		static bool hasMultiByteUTF8(const std::string &text);
	public:
		explicit TextRenderer(resman::ResourceManager* resMan, ShaderManager* shdrManager);
		~TextRenderer();
		void PostRendererInit();
		FONTHANDLE LoadFont(const resman::ResourceRequest &request);
		STATICTEXTHANDLE CreateStaticText(FONTHANDLE handle, const std::string &text, FONTSIZE fontSize);
		void UpdateScreenSize(int width, int height);
		void DrawStaticText2D(STATICTEXTHANDLE hStaticText, GLfloat x, GLfloat y, GLfloat z = 0, uint32_t color = 0xFFFFFFFF, TEXTALIGNMENT alignment = TXTA_LEFT);
		void DrawDynamicText2D(FONTHANDLE hFont, const std::string &text, FONTSIZE fontSize, GLfloat x, GLfloat y, GLfloat z = 0, uint32_t color = 0xFFFFFFFF, TEXTALIGNMENT alignment = TXTA_LEFT);
		size_t GetStaticTextWidth(STATICTEXTHANDLE hStaticText);
		size_t GetDynamicTextWidth(FONTHANDLE hFont, const std::string &text, FONTSIZE fontSize);
		void ReleaseStaticText(STATICTEXTHANDLE hStaticText);
		void UnloadFont(FONTHANDLE hFont);
	};

}


