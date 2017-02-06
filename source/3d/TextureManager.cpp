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
			//	JPEG has a 2 byte header
			uint16_t shortMagic = *reinterpret_cast<uint16_t*>(data->data());
			switch (shortMagic)
			{
			case MAGIC_JPEG:
				return LoadJPEG(data._data);
			default:
				break;
			}
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

dv3d::GLTEXHANDLE dv3d::TextureManager::LoadJPEG(std::vector<uint8_t>& data)
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg::jpeg_mem_src(&cinfo, data.data(), data.size());
	int rc = jpeg_read_header(&cinfo, true);
	if (rc != 1)
	{
		LOG(WARNING) << "Corrupt or invalid JPEG";
		return INVALID_GLTEXHANDLE;
	}
	jpeg_start_decompress(&cinfo);
	int width = cinfo.output_width;
	int height = cinfo.output_height;
	int components = cinfo.output_components;
	size_t bitmapSz = width * height * components;
	std::unique_ptr<uint8_t[]> buf(new uint8_t[bitmapSz]);
	int rowStride = width * components;
	while (cinfo.output_scanline < height)
	{
		unsigned char* bufArray[1];
		bufArray[0] = buf.get() + (cinfo.output_scanline) * rowStride;
		jpeg_read_scanlines(&cinfo, bufArray, 1);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	GLuint tex;
	glGenTextures(1, &tex);
	auto handle = _textures.insert(tex);
	//	Attach
	glBindTexture(GL_TEXTURE_2D, tex);
	GLenum type = GL_RGB;
	if (components == 4)
	{
		type = GL_RGBA;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, type, GL_UNSIGNED_BYTE, buf.get());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	LOG(DEBUG) << "Loaded JPEG texture with texhandle " << handle << " GLTEXID " << tex;
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

void dv3d::jpeg::init_source(j_decompress_ptr cinfo)
{
	//	Empty
}

boolean dv3d::jpeg::fill_input_buffer(j_decompress_ptr cinfo)
{
	ERREXIT(cinfo, JERR_INPUT_EMPTY);
	return true;
}

void dv3d::jpeg::skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	jpeg_source_mgr* src = static_cast<jpeg_source_mgr*>(cinfo->src);
	if (num_bytes > 0)
	{
		src->next_input_byte += static_cast<size_t>(num_bytes);
		src->bytes_in_buffer -= static_cast<size_t>(num_bytes);
	} 
}

void dv3d::jpeg::term_source(j_decompress_ptr cinfo)
{
	//	Empty
}

void dv3d::jpeg::jpeg_mem_src(j_decompress_ptr cinfo, void* buffer, long nbytes)
{
	jpeg_source_mgr* src;
	if (cinfo->src == nullptr)
	{
		cinfo->src = static_cast<jpeg_source_mgr*>(
			(*cinfo->mem->alloc_small)(reinterpret_cast<j_common_ptr>(cinfo), 
				JPOOL_PERMANENT, 
				sizeof(jpeg_source_mgr))
		);
	}
	src = static_cast<jpeg_source_mgr*>(cinfo->src);
	src->init_source = init_source;
	src->fill_input_buffer = fill_input_buffer;
	src->skip_input_data = skip_input_data;
	src->resync_to_restart = jpeg_resync_to_restart;
	src->term_source = term_source;
	src->bytes_in_buffer = nbytes;
	src->next_input_byte = static_cast<JOCTET*>(buffer);
}


