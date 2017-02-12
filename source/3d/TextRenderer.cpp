#include "../stdafx.h"
#include "TextRenderer.h"


dv3d::FontEntry::FontEntry() : sizes()
{
}

dv3d::FontEntry::~FontEntry()
{
}

dv3d::Character::Character()
{
}

dv3d::FontSizeEntry::FontSizeEntry() : extChars()
{
	ZeroMemory(&asciiChars, sizeof(asciiChars));
}

dv3d::FontSizeEntry::~FontSizeEntry()
{
}

void dv3d::TextRenderer::InitFont(FontEntry* entry, FONTSIZE fontSize)
{
	if (entry->sizes.find(fontSize) == entry->sizes.end())
	{
		auto emp = entry->sizes.emplace(fontSize, FontSizeEntry());
		auto szEntry = &emp.first->second;
		FT_New_Size(entry->face, &szEntry->ftSize);
		FT_Activate_Size(szEntry->ftSize);
		FT_Set_Pixel_Sizes(entry->face, 0, fontSize);
		CreateAsciiAtlas(entry, szEntry, fontSize);
	}
}

void dv3d::TextRenderer::CreateAsciiAtlas(FontEntry* fontEntry, FontSizeEntry* entry, FONTSIZE fontSize)
{
	//	TODO Naive
	for (uint32_t i = 0; i < 128; ++i)
	{
		LoadExtGlyph(fontEntry, entry, fontSize, i);
	}
}

void dv3d::TextRenderer::LoadExtGlyph(FontEntry* fontEntry, FontSizeEntry* entry, FONTSIZE fontSize, uint32_t codepoint)
{
	auto face = fontEntry->face;
	if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER))
	{
		LOG(WARNING) << "Failed to load glyph for codepoint " << codepoint;
		return;
	}
	Character character;
	if (face->glyph->bitmap.width > 0)
	{
		glGenTextures(1, &character.extTexture);
		glBindTexture(GL_TEXTURE_2D, character.extTexture);
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	character.pxDimensions.x = face->glyph->bitmap.width;
	character.pxDimensions.y = face->glyph->bitmap.rows;
	character.pxBearing.x = face->glyph->bitmap_left;
	character.pxBearing.y = face->glyph->bitmap_top;
	character.pxAdvance = face->glyph->advance.x;
	entry->extChars.insert(std::pair<uint32_t, Character>(codepoint, character));
}

bool dv3d::TextRenderer::IsGlyphLoaded(FontEntry* fontEntry, FONTSIZE fontSize, uint32_t codepoint)
{
	auto itSize = fontEntry->sizes.find(fontSize);
	if (itSize != fontEntry->sizes.end())
	{
//		if (codepoint < 128)
//		{
//			return true;
//		}
		auto fntSzEntry = itSize->second;
		auto itPt = fntSzEntry.extChars.find(codepoint);
		return itPt != fntSzEntry.extChars.end();
	}
	return false;
}

bool dv3d::TextRenderer::hasMultiByteUTF8(const std::string& text)
{
	for (auto c : text)
	{
		if (c & 0x80)
		{
			return true;
		}
	}
	return false;
}

dv3d::TextRenderer::TextRenderer(resman::ResourceManager* resMan, ShaderManager* shdrManager) : _fonts(16)
{
	_resManager = resMan;
	_shdrManager = shdrManager;
	if (FT_Init_FreeType(&ft))
	{
		LOG(WARNING) << "Failed to init freetype";
		throw "Failed to init FT";
	}
}

dv3d::TextRenderer::~TextRenderer()
{
	FT_Done_FreeType(ft);
}

void dv3d::TextRenderer::PostRendererInit()
{
	//	Set up VAOs
	glGenVertexArrays(1, &quadVertexArray);
	glGenBuffers(1, &quadVertexBuffer);
	glBindVertexArray(quadVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
	//	Vertex format is POS(X, Y, Z) COLOR(R, G, B, A) TEXCOORD(U, V)
	size_t stride = 3 + 4 + 2;
	size_t strideSz = sizeof(GLfloat) * stride;
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * stride, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideSz, nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, strideSz, reinterpret_cast<void*>(sizeof(float) * 3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, strideSz, reinterpret_cast<void*>(sizeof(float) * (3 + 4)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//	Load shaders
	h2dTextShader = _shdrManager->NewProgram();
	_shdrManager->AttachAndCompileShader(h2dTextShader, ppac::TPUID(0x0107, 0x0001, 0x00000001));
	_shdrManager->AttachAndCompileShader(h2dTextShader, ppac::TPUID(0x0108, 0x0001, 0x00000001));
	if (!_shdrManager->LinkAndFinishProgram(h2dTextShader))
	{

		LOG(WARNING) << "Shader compilation failed";
	}
}

dv3d::FONTHANDLE dv3d::TextRenderer::LoadFont(const resman::ResourceRequest& request)
{
	auto data = _resManager->GetResource(request);
	if (!data._present)
	{
		LOG(WARNING) << "Unable to load font data";
		return INVALID_FONTHANDLE;
	}
	auto hFont = _fonts.insert(std::make_unique<FontEntry>());
	auto entry = _fonts[hFont].get();
	//	Move the loaded data to the entry so it has ownership
	entry->fontData = std::move(data._data);
	if (FT_New_Memory_Face(ft,
		entry->fontData.data(),
		entry->fontData.size(),
		0,
		&entry->face))
	{
		LOG(WARNING) << "Unable to load 0-face";
		_fonts.erase(hFont);
		return INVALID_FONTHANDLE;
	}
	return hFont;
}

void dv3d::TextRenderer::UpdateScreenSize(int width, int height)
{
	screenWidth = width;
	screenHeight = height;
	projView = glm::ortho(0.0F, float(screenWidth), 0.0F, float(screenHeight));
}

void dv3d::TextRenderer::DrawDynamicText2D(FONTHANDLE hFont, const std::string& text, FONTSIZE fontSize, GLfloat x, GLfloat y, GLfloat z, uint32_t color, TEXTALIGNMENT alignment)
{
	auto font = _fonts[hFont].get();
	InitFont(font, fontSize);
	auto fontSz = font->sizes[fontSize];
	//	Optimize for the common case, single byte UTF8 means we don't need to expand to UTF32 and can also take advantage of the ASCII texture atlas.
	bool hasMultibyte = hasMultiByteUTF8(text);
	GLfloat red = GLfloat((color >> 16) & 0xFF) / 255.0F;
	GLfloat green = GLfloat((color >> 8) & 0xFF) / 255.0F;
	GLfloat blue = GLfloat(color & 0xFF) / 255.0F;
	GLfloat alpha = GLfloat((color >> 24) & 0xFF) / 255.0F;
	glUseProgram(_shdrManager->Get(h2dTextShader));
	glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(projView));
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(quadVertexArray);

	if (!hasMultibyte)
	{
		//	TODO right now naive solution, everything is ext
		std::string::const_iterator c;
		for (c = text.begin(); c != text.end(); ++c)
		{
			Character ch = fontSz.extChars[uint32_t(*c) & 0xFFu];
			if (ch.pxDimensions.x == 0 || ch.extTexture == 0)
			{
				x += (ch.pxAdvance >> 6);
				continue;
			}

			GLfloat xPos = x + ch.pxBearing.x;
			GLfloat yPos = y - (ch.pxDimensions.y - ch.pxBearing.y);
			GLfloat w = ch.pxDimensions.x;
			GLfloat h = ch.pxDimensions.y;
			GLfloat verts[4][3 + 4 + 2] = {
				{xPos, yPos , z, red, green, blue, alpha, 0.0F, 0.0F},
				{xPos, yPos + h, z, red, green, blue, alpha, 0.0F, 1.0F},
				{xPos + w, yPos + h, z, red, green, blue, alpha, 1.0F, 1.0F},
				{xPos + w, yPos, z, red, green, blue, alpha, 1.0F, 0.0F}
			};
			glBindTexture(GL_TEXTURE_2D, ch.extTexture);
			glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			x += (ch.pxAdvance >> 6);
		}
	}
	else
	{
		std::vector<uint32_t> asUtf32;
		const char* cstr = text.c_str();
		size_t len = strlen(cstr);
		std::unique_ptr<char*> buf = std::make_unique<char*>(new char[len + 1]);
		auto bufPtr = *buf.get();
		std::copy_n(cstr, len + 1, bufPtr);
		utf8::unchecked::utf8to32(bufPtr, bufPtr + len, std::back_inserter(asUtf32));
		buf.reset();
		for (auto codepoint : asUtf32)
		{
			Character ch;
			auto it = fontSz.extChars.find(codepoint);
			if (it == fontSz.extChars.end())
			{
				LoadExtGlyph(font, &font->sizes.at(fontSize), fontSize, codepoint);
				ch = fontSz.extChars[codepoint];
			}
			else
			{
				ch = it->second;
			}
			if (ch.pxDimensions.x == 0 || ch.extTexture == 0)
			{
				x += (ch.pxAdvance >> 6);
				continue;
			}
			GLfloat xPos = x + ch.pxBearing.x;
			GLfloat yPos = y - (ch.pxDimensions.y - ch.pxBearing.y);
			GLfloat w = ch.pxDimensions.x;
			GLfloat h = ch.pxDimensions.y;
			GLfloat verts[4][3 + 4 + 2] = {
				{ xPos, yPos , z, red, green, blue, alpha, 0.0F, 0.0F },
				{ xPos, yPos + h, z, red, green, blue, alpha, 0.0F, 1.0F },
				{ xPos + w, yPos + h, z, red, green, blue, alpha, 1.0F, 1.0F },
				{ xPos + w, yPos, z, red, green, blue, alpha, 1.0F, 0.0F }
			};
			glBindTexture(GL_TEXTURE_2D, ch.extTexture);
			glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			x += (ch.pxAdvance >> 6);
		}

	}
	glBindVertexArray(0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}




