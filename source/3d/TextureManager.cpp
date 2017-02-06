#include "../stdafx.h"
#include "TextureManager.h"

dv3d::TextureManager::TextureManager(resman::ResourceManager* resManager) : _textures(2048)
{
	assert(resManager != nullptr);
	_resManager = resManager;
}

dv3d::TextureManager::~TextureManager()
{
}

dv3d::GLTEXHANDLE dv3d::TextureManager::Load(const resman::ResourceRequest& request)
{
	auto data = _resManager->GetResource(request);
	if (data)
	{
		//	Determine the file type
		//	This can always be done by the first 4 bytes of the data (for all the formats we support)
		uint32_t magic = *reinterpret_cast<uint32_t*>(data->data());
		switch (magic)
		{
		case MAGIC_DDS:
			return LoadDDS(data._data);
		default:
			//	Unsupported
			return INVALID_GLTEXHANDLE;
		}
	}
	else
	{
		return INVALID_GLTEXHANDLE;
	}
}

dv3d::GLTEXHANDLE dv3d::TextureManager::LoadDDS(std::vector<uint8_t>& data)
{
	uint32_t* asUint32 = reinterpret_cast<uint32_t*>(data.data());
	uint32_t* itr = asUint32;
	uint32_t magic = *itr;
	if (magic != 0x20534444)
	{
		LOG(WARNING) << "Invalid splash texture: not a DDS";
		return INVALID_GLTEXHANDLE;
	}
	itr += 1;
	DDS_HEADER header = *reinterpret_cast<DDS_HEADER*>(itr);
	if (header.dwSize != 124)
	{
		LOG(WARNING) << "Corrupt or invalid DDS: invalid dwSize, expected 124, got " << header.dwSize;
		return INVALID_GLTEXHANDLE;
	}
	size_t bufSize = (header.dwMipMapCount > 1) ? header.dwPitchOrLinearSize * 2 : header.dwPitchOrLinearSize;
	std::unique_ptr<uint8_t[]> buf(new uint8_t[bufSize]);
	std::copy_n(&data[128], bufSize, stdext::checked_array_iterator<uint8_t*>(buf.get(), bufSize));
	GLuint glTexFormat;
	switch (header.ddspf.dwFourCC)
	{
	case DDS_FOURCC_DXT1:
		glTexFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case DDS_FOURCC_DXT3:
		glTexFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case DDS_FOURCC_DXT5:
		glTexFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;
	default:
		LOG(WARNING) << "Unknown FourCC " << header.ddspf.dwFourCC;
		return INVALID_GLTEXHANDLE;
	}
	size_t blockSize = (glTexFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	size_t width = header.dwWidth;
	size_t height = header.dwHeight;
	//	Generate a texture and grab it
	GLuint tex;
	glGenTextures(1, &tex);
	auto handle = _textures.insert(tex);
	//	Attach
	glBindTexture(GL_TEXTURE_2D, tex);
	size_t offset = 0;
	for (auto i = 0U; i <= header.dwMipMapCount && (width && height); ++i)
	{
		size_t imgSize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
		glCompressedTexImage2D(GL_TEXTURE_2D, i, glTexFormat, width, height, 0, imgSize, buf.get() + offset);
		width /= 2;
		height /= 2;
		offset += imgSize;
	}
	bool hasMipmaps = header.dwMipMapCount > 0;
	if (hasMipmaps)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (hasMipmaps)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	LOG(DEBUG) << "Loaded DDS texture with texhandle " << handle << " GLTEXID " << tex;
	return handle;
}

dv3d::TextureManager::GLTexHandleAndReference dv3d::TextureManager::LoadAndGet(const resman::ResourceRequest& request)
{
	GLTEXHANDLE handle = Load(request);
	return GLTexHandleAndReference(handle, Get(handle));
}

GLuint dv3d::TextureManager::Get(const GLTEXHANDLE& handle) const
{
	if (_textures.contains(handle)) {
		return _textures[handle];
	}
	return 0;
}

void dv3d::TextureManager::Unload(const GLTEXHANDLE& handle)
{
	if (_textures.contains(handle)) {
		auto tex = _textures[handle];
		glDeleteTextures(1, &tex);
		_textures.erase(handle);
		LOG(DEBUG) << "Deleted texhandle " << handle << " GLTEXID " << tex;
	}
	else
	{
		LOG(WARNING) << "Attempted to unload a texture that doesn't exist or has already been unloaded: texhandle " << handle;
	}
}
