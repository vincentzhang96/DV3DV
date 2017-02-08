#include "../stdafx.h"
#include "TextRenderer.h"

dv3d::FontWeightEntry::FontWeightEntry() : sizes()
{
}

dv3d::FontEntry::FontEntry() : weights()
{
}

dv3d::Character::Character()
{
}

dv3d::FontWeightSizeEntry::FontWeightSizeEntry() : extChars()
{
}

void dv3d::TextRenderer::InitFont(FontEntry& entry, FONTSIZE fontSize, FONTWEIGHT weight)
{

}

bool dv3d::TextRenderer::IsGlyphLoaded(FontEntry& fontEntry, FONTSIZE fontSize, FONTWEIGHT weight, uint32_t codepoint)
{
	auto itWeight = fontEntry.weights.find(weight);
	if (itWeight != fontEntry.weights.end())
	{
		auto fntWeightEntry = itWeight->second;
		auto itSize = fntWeightEntry.sizes.find(fontSize);
		if (itSize != fntWeightEntry.sizes.end())
		{
			if (codepoint < 128)
			{
				return true;
			}
			auto fntWtSzEntry = itSize->second;
			auto itPt = fntWtSzEntry.extChars.find(codepoint);
			return itPt != fntWtSzEntry.extChars.end();
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
		&entry->zeroFace))
	{
		LOG(WARNING) << "Unable to load 0-face";
		_fonts.erase(hFont);
		return INVALID_FONTHANDLE;
	}
	return hFont;
}




