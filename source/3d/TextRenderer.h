#pragma once
#include "../ResourceManager.h"
#include "ShaderManager.h"

namespace dv3d
{
#define INVALID_FONTHANDLE 0

#define SHDR_2D_TEXT_VERT ppac::TPUID(0x0107, 0x0001, 0x00000001)
#define SHDR_2D_TEXT_FRAG ppac::TPUID(0x0108, 0x0001, 0x00000001)

	typedef uint32_t FONTHANDLE;
	typedef uint32_t STATICTEXTHANDLE;
	typedef uint8_t FONTSIZE;

	struct Character;
	struct FontEntry;
	struct FontSizeEntry;
	struct TextOptions;

	struct FontEntry
	{
		std::unordered_map<FONTSIZE, FontSizeEntry> sizes;
		FT_Face face;
		std::vector<uint8_t> fontData;
		FontEntry();
		~FontEntry();
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
		GLuint asciiAtlasTexSize;
		Character asciiChars[128];
		std::unordered_map<uint32_t, Character> extChars;
		FT_Size ftSize;
		FontSizeEntry();
		~FontSizeEntry();
	};


//	Text alignment set flag
#define TEXTOPTION_ALIGNMENT 1
//	Tracking set flag
#define TEXTOPTION_TRACKING 2

	struct TextOptions
	{
		//	Bitmask of which options are active. See TEXTOPTION_* macros
		uint8_t flags = 0;
		//	Text alignment
		TEXTALIGNMENT alignment = TXTA_LEFT;
		//	Text tracking
		int tracking = 0;

		//	Default constructor. Initializes to no settings set with 0 tracking and left aligned.
		TextOptions();

		//	Constructs with the given flags with 0 tracking and left aligned.
		TextOptions(int f);
	};

	TextOptions textOptionAlignment(TEXTALIGNMENT align);
	TextOptions textOptionTracking(int tracking);

	//	Static text is text rendered to a texture and drawn on a single quad
	struct StaticText
	{
		//	Width of the text quad
		GLfloat textWidth;
		//	Height of the text quad
		GLfloat textHeight;
		//	Quad texture ID
		GLuint textTexture;
		//	Text options used to create this StaticText
		TextOptions textOptions;
	};

	class TextRenderer
	{
		typedef std::unordered_map<resman::ResourceRequest, FONTHANDLE, resman::RESOURCEREQUESTHASH> ResRequestFontCache;
		//	Loaded fonts
		packed_freelist<std::unique_ptr<FontEntry>> _fonts;
		//	Cache already loaded ResourceRequests
		ResRequestFontCache _resourceToFontCache;
		//	Resource manager to load fonts from
		resman::ResourceManager* _resManager;
		//	Shader manager to use
		ShaderManager* _shdrManager;
		//	FreeType library instance
		FT_Library ft;
		//	Width of the screen, for 2D drawing
		int screenWidth;
		//	Height of the screen, for 2D drawing
		int screenHeight;
		//	Projection matrix for 2D drawing
		glm::fmat4 projView;
		//	VAO for single quad
		GLuint quadVertexArray = 0;
		//	VBO for single quad
		GLuint quadVertexBuffer = 0;
		//	VAO for ASCII drawing
		GLuint asciiQuadVertexArray = 0;
		//	VBO for ASCII drawing
		GLuint asciiQuadVertexBuffer = 0;
		//	Index buffer for ASCII drawing
		GLuint asciiQuadIndexBuffer = 0;
		//	Shader handle for 2D drawing
		GLPROGHANDLE h2dTextShader = 0;
		//	Constructed static text instances
		packed_freelist<StaticText> _staticText;

		//	Initializes a font for a given fontsize, creating its data structures and the ASCII atlas
		static void InitFont(FontEntry* entry,
			FONTSIZE fontSize
		);
		//	Creates an ASCII atlas for a given font size
		static void CreateAsciiAtlas(FontEntry* fontEntry, 
			FontSizeEntry* entry, 
			FONTSIZE fontSize
		);

		//	Loads an ext glyph (codepoint > 127) for a given font size
		static void LoadExtGlyph(FontEntry* fontEntry, 
			FontSizeEntry* entry, 
			FONTSIZE fontSize, 
			uint32_t codepoint
		);

		//	Checks if the glyph for the given codepoint is loaded
		static bool IsGlyphLoaded(FontEntry* fontEntry, 
			FONTSIZE fontSize, 
			uint32_t codepoint
		);

		//	Checks if the string contains multibyte UTF8 characters
		static bool hasMultiByteUTF8(const std::string &text);

		//	Buffers an ASCII character into the given buffer at the given position
		static bool BufferASCIICharacter(GLfloat x, GLfloat y, GLfloat z, 
			FontSizeEntry* fontSz, Character* ch, 
			std::vector<GLfloat>* vertexData, std::vector<GLushort>* indices, 
			size_t vertexNumber
		);

		//	Renders the ASCII VAO with the given vertex data populated by BufferASCIICharacter calls
		void RenderASCIICharacterBuffer(std::vector<GLfloat>* vertexData, std::vector<GLushort>* indices) const;

		//	Counts the number of newlines in a given string
		static size_t CountNewlines(const std::string &text);
		
		//	Sets the line offset for the current line
		static GLfloat GetLineOffset(TextOptions options, GLfloat xOrigin, std::vector<GLfloat> &lineWidths, size_t lineNum);
	public:
		//	Constructor
		explicit TextRenderer(resman::ResourceManager* resMan, ShaderManager* shdrManager);

		//	Destructor
		~TextRenderer();

		//	Creates various internal OpenGL objects. Should be called once the OpenGL context is created (BUT NOT BEFORE)
		void PostRendererInit();

		//	Loads a given font
		FONTHANDLE LoadFont(const resman::ResourceRequest &request);

		//	Creates a static text instance that can be drawn by passing the returned STATICTEXTHANDLE to DrawStaticText2D()
		//	Be sure to destroy it after it is no longer needed using ReleaseStaticText()
		STATICTEXTHANDLE CreateStaticText(FONTHANDLE handle, const std::string &text, FONTSIZE fontSize, TextOptions options = 0);
		
		//	Update the screen size. Call after viewport resize
		void UpdateScreenSize(int width, int height);

		//	Draws a static text instance at the given position with the optionally specified color (defaults to white)
		void DrawStaticText2D(STATICTEXTHANDLE hStaticText, GLfloat x, GLfloat y, GLfloat z = 0, uint32_t color = 0xFFFFFFFF);

		//	Draws the given text at the given position with the optionally specified color and text options. Defaults to white, 0 leading, left aligned.
		void DrawDynamicText2D(FONTHANDLE hFont, const std::string &text, FONTSIZE fontSize, GLfloat x, GLfloat y, GLfloat z = 0, uint32_t color = 0xFFFFFFFF, TextOptions options = 0);

		//	Gets the pixel width of the given static text instance
		GLfloat GetStaticTextWidth(STATICTEXTHANDLE hStaticText) const;
		
		//	Gets the pixel width of the given text with the optionally specified text options. Defaults to 0 leading, left aligned.
		GLfloat GetDynamicTextWidth(FONTHANDLE hFont, const std::string &text, FONTSIZE fontSize, TextOptions options = 0) const;

		//	Gets the pixel width of the given text per line with the optionall specified text options. The return value will be the maximum width of the text (max of all the lines), 
		//	and each vector entry is the width of the corresponding line. Defaults to 0 leading, left aligned (alignment does not affect text width).
		GLfloat GetDynamicTextWidthPerLine(OUT std::vector<GLfloat> &out, FONTHANDLE hFont, const std::string &text, FONTSIZE fontSize, TextOptions options = 0) const;
		
		//	Releases a given static text instance. The STATICTEXTHANDLE should be discarded and should not be passed to other StaticText methods.
		void ReleaseStaticText(STATICTEXTHANDLE hStaticText);

		//	Unloads a given loaded font.
		void UnloadFont(FONTHANDLE hFont);
	};

}


