#include "../stdafx.h"
#include "TextRenderer.h"


dv3d::FontEntry::FontEntry() : sizes()
{
}

dv3d::FontEntry::~FontEntry()
{
	FT_Done_Face(face);
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
	FT_Done_Size(ftSize);
}

void dv3d::TextRenderer::InitFont(FontEntry& entry, FONTSIZE fontSize)
{
	if (entry.sizes.find(fontSize) == entry.sizes.end())
	{
		auto emp = entry.sizes.emplace(fontSize, FontSizeEntry());
		auto szEntry = &emp.first->second;
		FT_New_Size(entry.face, &szEntry->ftSize);
		FT_Activate_Size(szEntry->ftSize);
		FT_Set_Pixel_Sizes(entry.face, 0, fontSize);
		CreateAsciiAtlas(entry, *szEntry, fontSize);
	}
}

void dv3d::TextRenderer::CreateAsciiAtlas(FontEntry& fontEntry, FontSizeEntry& entry, FONTSIZE fontSize)
{

}

void dv3d::TextRenderer::LoadExtGlyph(FontEntry& fontEntry, FontSizeEntry& entry, FONTSIZE fontSize, uint32_t codepoint)
{
	auto face = fontEntry.face;
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
	entry.extChars.insert(std::pair<uint32_t, Character>(codepoint, character));
}

bool dv3d::TextRenderer::IsGlyphLoaded(FontEntry& fontEntry, FONTSIZE fontSize, uint32_t codepoint)
{
	auto itSize = fontEntry.sizes.find(fontSize);
	if (itSize != fontEntry.sizes.end())
	{
		if (codepoint < 256)
		{
			return true;
		}
		auto fntSzEntry = itSize->second;
		auto itPt = fntSzEntry.extChars.find(codepoint);
		return itPt != fntSzEntry.extChars.end();
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




