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

dv3d::TextOptions::TextOptions()
{
	flags = 0;
}

dv3d::TextOptions::TextOptions(int f)
{
	flags = f;
}

dv3d::TextOptions dv3d::textOptionAlignment(TEXTALIGNMENT align)
{
	TextOptions options(TEXTOPTION_ALIGNMENT);
	options.alignment = align;
	return options;
}

dv3d::TextOptions dv3d::textOptionTracking(int tracking)
{
	TextOptions options(TEXTOPTION_TRACKING);
	options.tracking = tracking;
	return options;
}

void dv3d::TextRenderer::InitFont(FontEntry* entry, FONTSIZE fontSize)
{
	if (entry->sizes.find(fontSize) == entry->sizes.end())
	{
		LOG(DEBUG) << "Init font " << entry->face->style_name << " sz " << unsigned int(fontSize);
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
	FT_Activate_Size(entry->ftSize);
	glGenTextures(1, &entry->asciiAtlasTex);
	glBindTexture(GL_TEXTURE_2D, entry->asciiAtlasTex);
	//	Get the metrics for all characters first
	glm::ivec2 maxSize(0, 0);
	auto face = fontEntry->face;
	for (uint32_t i = 0; i < 128; ++i)
	{
		Character* character = &entry->asciiChars[i];
		if (FT_Load_Char(face, i, FT_LOAD_RENDER))
		{
			LOG(WARNING) << "Failed to load glyph metrics for codepoint " << i;
			continue;
		}
		character->pxDimensions.x = face->glyph->bitmap.width;
		character->pxDimensions.y = face->glyph->bitmap.rows;
		if (character->pxDimensions.x > maxSize.x)
		{
			maxSize.x = character->pxDimensions.x;
		}
		if (character->pxDimensions.y > maxSize.y)
		{
			maxSize.y = character->pxDimensions.y;
		}
		character->pxBearing.x = face->glyph->bitmap_left;
		character->pxBearing.y = face->glyph->bitmap_top;
		character->pxAdvance = face->glyph->advance.x;
	}
	//	Calculate the number of columns we need, based on the area required and the max width encountered
	//	We should aim to hit 12 glyphs per row, so
	int minReqForTwelve = maxSize.x * 12;
	entry->asciiAtlasTexSize = minReqForTwelve;
	//	Alloc texture atlas
	std::vector<uint8_t> empty(minReqForTwelve * minReqForTwelve, 0);
	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RED,
		minReqForTwelve,
		minReqForTwelve,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		empty.data()
	);
	//	Start shoving in glyphs
	//	TODO For now we're just shoving in 12 glyphs per row, but later on we can optimize packing
	for (uint32_t codepoint = 0; codepoint < 128; ++codepoint)
	{
		int rowNum = codepoint / 12;
		int colNum = codepoint % 12;
		Character* ch = &entry->asciiChars[codepoint];
		if (ch->pxDimensions.x == 0)
		{
			continue;
		}
		if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER))
		{
			LOG(WARNING) << "Failed to load glyph bitmap for codepoint " << codepoint;
			continue;
		}
		glTexSubImage2D(GL_TEXTURE_2D,
			0,
			colNum * maxSize.x,
			rowNum * fontSize,
			ch->pxDimensions.x,
			ch->pxDimensions.y,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		ch->asciiUVAtlasStart.x = float(colNum * maxSize.x);
		ch->asciiUVAtlasStart.y = minReqForTwelve - float(rowNum * fontSize);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void dv3d::TextRenderer::LoadExtGlyph(FontEntry* fontEntry, FontSizeEntry* entry, FONTSIZE fontSize, uint32_t codepoint)
{
	auto face = fontEntry->face;
	FT_Activate_Size(entry->ftSize);
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
		if (codepoint < 128)
		{
			return true;
		}
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

bool dv3d::TextRenderer::BufferASCIICharacter(GLfloat x, GLfloat y, GLfloat z, FontSizeEntry* fontSz, Character* ch, std::vector<GLfloat>* vertexData, std::vector<GLushort>* indices, size_t vertexNumber)
{
	if (ch->pxDimensions.x == 0)
	{
		return false;
	}
	GLfloat xPos = x + ch->pxBearing.x;
	GLfloat yPos = y - (ch->pxDimensions.y - ch->pxBearing.y);
	GLfloat w = ch->pxDimensions.x;
	GLfloat h = ch->pxDimensions.y;
	GLfloat uStart = ch->asciiUVAtlasStart.x / float(fontSz->asciiAtlasTexSize);
	GLfloat vEnd = ch->asciiUVAtlasStart.y / float(fontSz->asciiAtlasTexSize);
	GLfloat uSz = w / float(fontSz->asciiAtlasTexSize);
	GLfloat vSz = h / float(fontSz->asciiAtlasTexSize);
	GLfloat uEnd = uStart + uSz;
	GLfloat vStart = vEnd - vSz;
	GLfloat verts[4 * (3 + 2)] = {
		xPos, yPos , z,
		uStart, vStart,

		xPos, yPos + h, z,
		uStart, vEnd,

		xPos + w, yPos + h, z,
		uEnd, vEnd,

		xPos + w, yPos, z,
		uEnd, vStart
	};
	vertexData->insert(vertexData->end(), verts, verts + (4 * (3 + 2)));
	GLfloat vertIndex[6] = {
		vertexNumber, vertexNumber + 1, vertexNumber + 2,
		vertexNumber, vertexNumber + 2, vertexNumber + 3
	};
	indices->insert(indices->end(), vertIndex, vertIndex + 6);
	return true;
}

void dv3d::TextRenderer::RenderASCIICharacterBuffer(std::vector<GLfloat>* vertexData, std::vector<GLushort>* indices)
{
	glBindVertexArray(asciiQuadVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, asciiQuadVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertexData->size(), vertexData->data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asciiQuadIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(GLushort), indices->data(), GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLES, indices->size(), GL_UNSIGNED_SHORT, nullptr);
	++_statistics.asciiBatchesDrawn;
}

size_t dv3d::TextRenderer::CountLines(const std::string &text)
{
	size_t ret = 1;
	std::string::const_iterator iter;
	for (iter = text.begin(); iter != text.end(); ++iter)
	{
		ret += (*iter == '\n');
	}
	return ret;
}

dv3d::TextRenderer::TextRenderer(resman::ResourceManager* resMan, ShaderManager* shdrManager) : _fonts(64), _staticText(8192)
{
	_resManager = resMan;
	_shdrManager = shdrManager;
	if (FT_Init_FreeType(&ft))
	{
		LOG(WARNING) << "Failed to init freetype";
		throw "Failed to init FT";
	}
	screenWidth = 0;
	screenHeight = 0;
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
	//	Vertex format is POS(X, Y, Z) TEXCOORD(U, V)
	size_t stride = 3 + 2;
	size_t strideSz = sizeof(GLfloat) * stride;
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * stride, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideSz, nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, strideSz, reinterpret_cast<void*>(sizeof(float) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//	VAO for ASCII atlas drawing
	glGenVertexArrays(1, &asciiQuadVertexArray);
	glBindVertexArray(asciiQuadVertexArray);
	glGenBuffers(1, &asciiQuadVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, asciiQuadVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * stride, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideSz, nullptr);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, strideSz, reinterpret_cast<void*>(sizeof(float) * 3));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenBuffers(1, &asciiQuadIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, asciiQuadIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 6, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//	Load shaders
	h2dTextShader = _shdrManager->NewProgram();
	_shdrManager->AttachAndCompileShader(h2dTextShader, SHDR_2D_TEXT_VERT);
	_shdrManager->AttachAndCompileShader(h2dTextShader, SHDR_2D_TEXT_FRAG);
	if (!_shdrManager->LinkAndFinishProgram(h2dTextShader))
	{
		LOG(WARNING) << "2D text shader compilation failed";
		throw "Shader compilation failed";
	}
}

dv3d::FONTHANDLE dv3d::TextRenderer::LoadFont(const resman::ResourceRequest& request)
{
	//	Check for cached first
	auto it = _resourceToFontCache.find(request);
	if (it != _resourceToFontCache.end())
	{
		return it->second;
	}

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
	//	Preload common font sizes
	FONTSIZE common[] = { 12, 14, 16, 18, 24 };
	for (auto sz : common)
	{
		InitFont(entry, sz);
	}
	_resourceToFontCache[request] = hFont;
	return hFont;
}

dv3d::STATICTEXTHANDLE dv3d::TextRenderer::CreateStaticText(FONTHANDLE hFont, const std::string& text, FONTSIZE fontSize, TextOptions options)
{
	GLfloat tracking = 0;
	if (options.flags & TEXTOPTION_TRACKING)
	{
		tracking = (options.tracking / 1000.0F) * fontSize;
	}
	GLfloat x = 0;
	GLfloat y = 0;
	GLfloat xOrigin = 0;
	auto font = _fonts[hFont].get();
	InitFont(font, fontSize);
	auto fontSz = &font->sizes[fontSize];
	GLfloat width = GetDynamicTextWidth(hFont, text, fontSize, options) + 1.0F;	//	Add padding pixel
	
	if (options.flags & TEXTOPTION_ALIGNMENT)
	{
		switch (options.alignment)
		{
		case TXTA_RIGHT:
			xOrigin = width;
			break;
		case TXTA_CENTER:
			xOrigin = width / 2.0F;
			break;
		case TXTA_LEFT:
		default:
			break;
		}
	}
	size_t numLines = CountLines(text);
	GLfloat height = (numLines + 1) * fontSize;
	StaticText stext;
	stext.size = fontSize;
	stext.textHeight = height;
	stext.textWidth = width;
	stext.textOptions = options;
	glGenTextures(1, &stext.textTexture);
	glBindTexture(GL_TEXTURE_2D, stext.textTexture);
	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RED,
		stext.textWidth,
		stext.textHeight,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		nullptr
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	std::vector<uint32_t> asUtf32;
	size_t len = text.length();
	std::vector<GLfloat> lineWidths;
	GLfloat maxWidth = GetDynamicTextWidthPerLine(lineWidths, hFont, text, fontSize, options);
	size_t lineNum = 0;
	x = GetLineOffset(options, xOrigin, lineWidths, lineNum);
	utf8::unchecked::utf8to32(text.data(), text.data() + len, std::back_inserter(asUtf32));
	for (auto codepoint : asUtf32)
	{
		if (codepoint == 0x0A)
		{
			y += fontSize;
			++lineNum;
			x = GetLineOffset(options, xOrigin, lineWidths, lineNum);
			continue;
		}
		Character ch;
		bool isExt = codepoint > 0x7F;
		if (!isExt)
		{
			ch = fontSz->asciiChars[codepoint];
		}
		else
		{
			auto it = fontSz->extChars.find(codepoint);
			if (it != fontSz->extChars.end())
			{
				ch = it->second;
			}
			else
			{
				LOG(WARNING) << "GetDynamicTextWidth should have loaded any unloaded glyphs for CreateStaticText!";
				throw "Glyph not loaded";
			}
		}
		if (ch.pxDimensions.x != 0)
		{
			GLint srcXStart = 0;
			GLint srcYStart = 0;
			GLint destXStart;
			GLint destYStart;
			GLuint textureId;
			if (!isExt)
			{
				GLfloat asciiAtlasTexSize = GLfloat(fontSz->asciiAtlasTexSize);
				srcXStart = ch.asciiUVAtlasStart.x;
				srcYStart = (asciiAtlasTexSize - ch.asciiUVAtlasStart.y);
				textureId = fontSz->asciiAtlasTex;
			}
			else
			{
				textureId = ch.extTexture;
			}
			destXStart = x + ch.pxBearing.x;
			destYStart = y + (fontSize - ch.pxBearing.y);
			if (destXStart + ch.pxDimensions.x > stext.textWidth)
			{
				LOG(WARNING) << "codepoint " << codepoint << " will go past bounds " << stext.textWidth << ": " << (destXStart + ch.pxDimensions.x);
				continue;
			}
			glCopyImageSubData(
				textureId, GL_TEXTURE_2D, 0,
				srcXStart, srcYStart, 0,
				stext.textTexture, GL_TEXTURE_2D, 0,
				destXStart, destYStart, 0,
				ch.pxDimensions.x, ch.pxDimensions.y, 1
			);
		}
		x += (ch.pxAdvance >> 6) + tracking;
	}
	return _staticText.insert(stext);
}

void dv3d::TextRenderer::UpdateScreenSize(int width, int height)
{
	screenWidth = width;
	screenHeight = height;
	projView = glm::ortho(0.0F, float(screenWidth), 0.0F, float(screenHeight));
}

void dv3d::TextRenderer::DrawStaticText2D(STATICTEXTHANDLE hStaticText, GLfloat x, GLfloat y, GLfloat z, uint32_t color)
{
	StaticText text = _staticText[hStaticText];
	GLfloat red = GLfloat((color >> 16) & 0xFF) / 255.0F;
	GLfloat green = GLfloat((color >> 8) & 0xFF) / 255.0F;
	GLfloat blue = GLfloat(color & 0xFF) / 255.0F;
	GLfloat alpha = GLfloat((color >> 24) & 0xFF) / 255.0F;
	glUseProgram(_shdrManager->Get(h2dTextShader));
	glUniform4f(2, red, green, blue, alpha);
	glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(projView));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text.textTexture);
	std::vector<GLfloat> vec = { text.textWidth };
	x = GetLineOffset(text.textOptions, x, vec, 0);
	GLfloat verts[4][3 + 2] = {
		{ x, y - text.textHeight / 2.0F, z, 0.0F, 0.0F },
		{ x, y + text.textHeight / 2.0F, z, 0.0F, 1.0F },
		{ x + text.textWidth, y + text.textHeight / 2.0F, z, 1.0F, 1.0F },
		{ x + text.textWidth, y - text.textHeight / 2.0F, z, 1.0F, 0.0F }
	};
	glBindVertexArray(quadVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	++_statistics.staticTextsDrawn;
}

GLfloat dv3d::TextRenderer::GetLineOffset(dv3d::TextOptions options, GLfloat xOrigin, std::vector<GLfloat> &lineWidths, size_t lineNum)
{
	if (options.flags & TEXTOPTION_ALIGNMENT)
	{
		switch (options.alignment)
		{
		case TXTA_RIGHT:
			return xOrigin - lineWidths[lineNum];
			break;
		case TXTA_CENTER:
			return xOrigin - (lineWidths[lineNum] / 2.0F);
			break;
		case TXTA_LEFT:
		default:
			return xOrigin;
			break;
		}
	}
	return xOrigin;
}

void dv3d::TextRenderer::DrawDynamicText2D(FONTHANDLE hFont, const std::string& text, const FONTSIZE fontSize, GLfloat x, GLfloat y, GLfloat z, uint32_t color, TextOptions options)
{
	GLfloat xOrigin = x;
	GLfloat yOrigin = y;
	auto font = _fonts[hFont].get();
	InitFont(font, fontSize);
	auto fontSz = &font->sizes[fontSize];
	//	Optimize for the common case, single byte UTF8 means we don't need to expand to UTF32 and can also take advantage of the ASCII texture atlas.
	bool hasMultibyte = hasMultiByteUTF8(text);
	GLfloat red = GLfloat((color >> 16) & 0xFF) / 255.0F;
	GLfloat green = GLfloat((color >> 8) & 0xFF) / 255.0F;
	GLfloat blue = GLfloat(color & 0xFF) / 255.0F;
	GLfloat alpha = GLfloat((color >> 24) & 0xFF) / 255.0F;
	glUseProgram(_shdrManager->Get(h2dTextShader));
	glUniform4f(2, red, green, blue, alpha);
	glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(projView));
	glActiveTexture(GL_TEXTURE0);

	GLfloat tracking = 0;
	if (options.flags & TEXTOPTION_TRACKING)
	{
		tracking = (options.tracking / 1000.0F) * fontSize;
	}

	std::vector<GLfloat> lineWidths;
	GLfloat maxWidth = GetDynamicTextWidthPerLine(lineWidths, hFont, text, fontSize, options);
	size_t lineNum = 0;
	x = GetLineOffset(options, xOrigin, lineWidths, lineNum);
	if (!hasMultibyte)
	{
		//	Text consists only of the first 128 characters (0-127)
		//	Use ASCII texture atlas
		std::string::const_iterator c;
		std::vector<GLfloat> vertexData(0);
		std::vector<GLushort> indices(0);
		size_t numQuads = 0;
		size_t vertexNumber = 0;
		for (c = text.begin(); c != text.end(); ++c)
		{
			if (*c == 0x0A)
			{
				y -= fontSize;
				++lineNum;
				x = GetLineOffset(options, xOrigin, lineWidths, lineNum);
				continue;
			}
			Character ch = fontSz->asciiChars[uint32_t(*c) & 0xFFu];
			if (BufferASCIICharacter(x, y, z, fontSz, &ch, &vertexData, &indices, vertexNumber))
			{
				++numQuads;
				vertexNumber += 4;
			}
			x += (ch.pxAdvance >> 6) + tracking;
		}
		glBindTexture(GL_TEXTURE_2D, fontSz->asciiAtlasTex);
		RenderASCIICharacterBuffer(&vertexData, &indices);
	}
	else
	{
		//	For this part we will batch ASCII calls in one pass
		//	and ext chars in another pass. The ext chars will be batched individually as well
		std::vector<uint32_t> asUtf32;
		size_t len = text.length();
		utf8::unchecked::utf8to32(text.data(), text.data() + len, std::back_inserter(asUtf32));
		std::vector<GLfloat> vertexData(0);
		std::vector<GLushort> indices(0);
		size_t numQuads = 0;
		size_t vertexNumber = 0;
		GLfloat asciiX = x;
		for (auto codepoint : asUtf32)
		{
			if (codepoint == 0x0A)
			{
				y -= fontSize;
				++lineNum;
				asciiX = GetLineOffset(options, xOrigin, lineWidths, lineNum);
				continue;
			}
			Character ch;
			bool isExt = codepoint > 0x7F;
			if (!isExt)
			{
				ch = fontSz->asciiChars[codepoint];
			}
			else
			{
				auto it = fontSz->extChars.find(codepoint);
				if (it != fontSz->extChars.end())
				{
					ch = it->second;
				}
				else
				{
					LoadExtGlyph(font, fontSz, fontSize, codepoint);
					ch = fontSz->extChars[codepoint];
				}
			}
			if (ch.pxDimensions.x == 0 || isExt)
			{
				asciiX += (ch.pxAdvance >> 6) + tracking;
				continue;
			}
			if (BufferASCIICharacter(asciiX, y, z, fontSz, &ch, &vertexData, &indices, vertexNumber))
			{
				++numQuads;
				vertexNumber += 4;
			}
			asciiX += (ch.pxAdvance >> 6) + tracking;
		}
		glBindTexture(GL_TEXTURE_2D, fontSz->asciiAtlasTex);
		RenderASCIICharacterBuffer(&vertexData, &indices);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(quadVertexArray);
		for (auto codepoint : asUtf32)
		{
			if (codepoint == 0x0A)
			{
				y -= fontSize;
				++lineNum;
				x = GetLineOffset(options, xOrigin, lineWidths, lineNum);
				continue;
			}
			Character ch;
			if (codepoint < 128)
			{
				ch = fontSz->asciiChars[codepoint];
			}
			else
			{
				auto it = fontSz->extChars.find(codepoint);
				if (it == fontSz->extChars.end())
				{
					LoadExtGlyph(font, fontSz, fontSize, codepoint);
					ch = fontSz->extChars[codepoint];
				}
				else
				{
					ch = it->second;
				}
			}
			if (ch.pxDimensions.x == 0 || codepoint < 128)
			{
				x += (ch.pxAdvance >> 6) + tracking;
				continue;
			}
			GLfloat xPos = x + ch.pxBearing.x;
			GLfloat yPos = y - (ch.pxDimensions.y - ch.pxBearing.y);
			GLfloat w = ch.pxDimensions.x;
			GLfloat h = ch.pxDimensions.y;
			//	Handle ext characters
			GLfloat verts[4][3 + 2] = {
				{ xPos, yPos , z, 0.0F, 0.0F },
				{ xPos, yPos + h, z, 0.0F, 1.0F },
				{ xPos + w, yPos + h, z, 1.0F, 1.0F },
				{ xPos + w, yPos, z, 1.0F, 0.0F }
			};
			glBindTexture(GL_TEXTURE_2D, ch.extTexture);
			glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			x += (ch.pxAdvance >> 6) + tracking;
			++_statistics.extGlyphsDrawn;
		}
	}
	glBindVertexArray(0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	++_statistics.dynamicTextsDrawn;
}

GLfloat dv3d::TextRenderer::GetStaticTextWidth(STATICTEXTHANDLE hStaticText) const
{
	StaticText text = _staticText[hStaticText];
	return text.textWidth;
}

GLfloat dv3d::TextRenderer::GetDynamicTextWidth(FONTHANDLE hFont, const std::string& text, FONTSIZE fontSize, TextOptions options) const
{
	std::vector<GLfloat> ret(2);
	return GetDynamicTextWidthPerLine(ret, hFont, text, fontSize, options);
}

GLfloat dv3d::TextRenderer::GetDynamicTextWidthPerLine(std::vector<GLfloat> &out, FONTHANDLE hFont, const std::string& text, FONTSIZE fontSize, TextOptions options) const
{
	GLfloat max = 0;
	GLfloat x = 0;
	auto font = _fonts[hFont].get();
	InitFont(font, fontSize);
	auto fontSz = &font->sizes[fontSize];
	GLfloat tracking = 0;
	if (options.flags & TEXTOPTION_TRACKING)
	{
		tracking = (options.tracking / 1000.0F) * fontSize;
	}
	//	Optimize for the common case, single byte UTF8 means we don't need to expand to UTF32 and can also take advantage of the ASCII texture atlas.
	bool hasMultibyte = hasMultiByteUTF8(text);
	if (!hasMultibyte)
	{
		//	Text consists only of the first 128 characters (0-127)
		//	Use ASCII texture atlas
		std::string::const_iterator c;
		for (c = text.begin(); c != text.end(); ++c)
		{
			if (*c == 0x0A)	//	newline
			{
				out.push_back(x);
				if (x > max)
				{
					max = x;
				}
				x = 0;
				continue;
			}
			Character ch = fontSz->asciiChars[uint32_t(*c) & 0xFFu];
			x += (ch.pxAdvance >> 6) + tracking;
		}
	}
	else
	{
		//	For this part we will batch ASCII calls in one pass
		//	and ext chars in another pass. The ext chars will be batched individually as well
		std::vector<uint32_t> asUtf32;
		size_t len = text.length();
		utf8::unchecked::utf8to32(text.data(), text.data() + len, std::back_inserter(asUtf32));
		for (auto codepoint : asUtf32)
		{
			if (codepoint == 0x0A)	//	newline
			{
				out.push_back(x);
				if (x > max)
				{
					max = x;
				}
				x = 0;
				continue;
			}
			Character ch;
			if (codepoint < 128)
			{
				ch = fontSz->asciiChars[codepoint];
			}
			else
			{
				auto it = fontSz->extChars.find(codepoint);
				if (it == fontSz->extChars.end())
				{
					LoadExtGlyph(font, fontSz, fontSize, codepoint);
					ch = fontSz->extChars[codepoint];
				}
				else
				{
					ch = it->second;
				}
			}
			x += (ch.pxAdvance >> 6) + tracking;
		}
	}
	if (x > max)
	{
		max = x;
	}
	//	Final line
	out.push_back(x);
	return max;
}

GLfloat dv3d::TextRenderer::GetStaticTextHeight(STATICTEXTHANDLE hStaticText) const
{
	auto text = _staticText[hStaticText];
	return text.textHeight - text.size;	//	Remove the padding
}

GLfloat dv3d::TextRenderer::GetDynamicTextHeight(FONTHANDLE hFont, const std::string& text, FONTSIZE fontSize, TextOptions options)
{
	size_t numNewlines = CountLines(text) + 1;
	return fontSize * numNewlines;
}

dv3d::TextOptions dv3d::TextRenderer::GetStaticTextOptions(STATICTEXTHANDLE hStaticText) const
{
	auto text = _staticText[hStaticText];
	return text.textOptions;
}

dv3d::FONTSIZE dv3d::TextRenderer::GetStaticTextSize(STATICTEXTHANDLE hStaticText) const
{
	auto text = _staticText[hStaticText];
	return text.size;
}

void dv3d::TextRenderer::ReleaseStaticText(STATICTEXTHANDLE hStaticText)
{
	StaticText text = _staticText[hStaticText];
	glDeleteTextures(1, &text.textTexture);
	_staticText.erase(hStaticText);
}

void dv3d::TextRenderer::UnloadFont(FONTHANDLE hFont)
{
	_fonts.erase(hFont);
	//	TODO invalidate cache entries
	//	Not really too important right now since this only gets destroyed at program exit
}

void dv3d::TextRenderer::FinishFrame()
{
	_statistics.Reset();
}
