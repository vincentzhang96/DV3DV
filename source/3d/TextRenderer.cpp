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
		ch->asciiUVAtlasStart.x = float(colNum * maxSize.x) / entry->asciiAtlasTexSize;
		ch->asciiUVAtlasStart.y = 1.0 - float(rowNum * fontSize) / entry->asciiAtlasTexSize;
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
	GLfloat uStart = ch->asciiUVAtlasStart.x;
	GLfloat vEnd = ch->asciiUVAtlasStart.y;
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

void dv3d::TextRenderer::DrawDynamicText2D(FONTHANDLE hFont, const std::string& text, FONTSIZE fontSize, GLfloat x, GLfloat y, GLfloat z, uint32_t color, TextOptions options)
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
	glUniform4f(2, red, green, blue, alpha);
	glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(projView));
	glActiveTexture(GL_TEXTURE0);

	GLfloat tracking = 0;
	if (options.flags & TEXTOPTION_TRACKING)
	{
		tracking = (options.tracking / 1000.0F) * fontSize;
	}

	if (!hasMultibyte)
	{
		//	Text consists only of the first 128 characters (0-127)
		//	Use ASCII texture atlas
		std::string::const_iterator c;
		glBindTexture(GL_TEXTURE_2D, fontSz.asciiAtlasTex);
		std::vector<GLfloat> vertexData(0);
		std::vector<GLushort> indices(0);
		size_t numQuads = 0;
		size_t vertexNumber = 0;
		for (c = text.begin(); c != text.end(); ++c)
		{
			Character ch = fontSz.asciiChars[uint32_t(*c) & 0xFFu];
			if (BufferASCIICharacter(x, y, z, &fontSz, &ch, &vertexData, &indices, vertexNumber))
			{
				++numQuads;
				vertexNumber += 4;
			}
			x += (ch.pxAdvance >> 6) + tracking;
		}
		RenderASCIICharacterBuffer(&vertexData, &indices);
	}
	else
	{
		glBindVertexArray(quadVertexArray);
		std::vector<uint32_t> asUtf32;
		size_t len = text.length();
		utf8::unchecked::utf8to32(text.data(), text.data() + len, std::back_inserter(asUtf32));
		for (auto codepoint : asUtf32)
		{
			Character ch;
			if (codepoint < 128)
			{
				ch = fontSz.asciiChars[codepoint];
			}
			else
			{
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
			}
			if (ch.pxDimensions.x == 0)
			{
				x += (ch.pxAdvance >> 6) + tracking;
				continue;
			}
			GLfloat xPos = x + ch.pxBearing.x;
			GLfloat yPos = y - (ch.pxDimensions.y - ch.pxBearing.y);
			GLfloat w = ch.pxDimensions.x;
			GLfloat h = ch.pxDimensions.y;
			if (codepoint < 128)
			{
				//	Handle ASCII atlasmap characters
				GLfloat uStart = ch.asciiUVAtlasStart.x;
				GLfloat vEnd = ch.asciiUVAtlasStart.y;
				GLfloat uSz = w / float(fontSz.asciiAtlasTexSize);
				GLfloat vSz = h / float(fontSz.asciiAtlasTexSize);
				GLfloat uEnd = uStart + uSz;
				GLfloat vStart = vEnd - vSz;
				GLfloat verts[4][3 + 2] = {
					{
						xPos, yPos , z,
						uStart, vStart
					},
					{
						xPos, yPos + h, z,
						uStart, vEnd
					},
					{
						xPos + w, yPos + h, z,
						uEnd, vEnd
					},
					{
						xPos + w, yPos, z,
						uEnd, vStart
					}
				};
				glBindTexture(GL_TEXTURE_2D, fontSz.asciiAtlasTex);
				glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
			}
			else
			{
				//	Handle ext characters
				GLfloat verts[4][3 + 2] = {
					{ xPos, yPos , z, 0.0F, 0.0F },
					{ xPos, yPos + h, z, 0.0F, 1.0F },
					{ xPos + w, yPos + h, z, 1.0F, 1.0F },
					{ xPos + w, yPos, z, 1.0F, 0.0F }
				};
				glBindTexture(GL_TEXTURE_2D, ch.extTexture);
				glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			x += (ch.pxAdvance >> 6) + tracking;
		}
	}
	glBindVertexArray(0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}




