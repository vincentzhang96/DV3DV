#pragma once
#include "../ResourceManager.h"

namespace dv3d
{
	typedef uint32_t GLTEXHANDLE;

#define INVALID_GLTEXHANDLE 0

#define MAGIC_DDS 0x20534444
#define MAGIC_JPEG 0xD8FF


	class TextureManager
	{
		typedef std::pair<GLTEXHANDLE, GLuint> GLTexHandleAndReference;
	private:
		packed_freelist<GLuint> _textures;
		resman::ResourceManager* _resManager;

	public:
		explicit TextureManager(resman::ResourceManager* resManager);
		~TextureManager();

		GLTEXHANDLE Load(const resman::ResourceRequest &request);
		GLTEXHANDLE LoadDDS(std::vector<uint8_t> &data);
		GLTEXHANDLE LoadJPEG(std::vector<uint8_t> &data);
		GLTexHandleAndReference LoadAndGet(const resman::ResourceRequest &request);
		GLuint Get(const GLTEXHANDLE &handle) const;
		void Unload(const GLTEXHANDLE &handle);
		void UnloadAllOf(const std::vector<GLTEXHANDLE> &handles);
		void UnloadAll();
	};

	//	JPEG loading helpers
	//	From http://stackoverflow.com/a/5299198
	namespace jpeg
	{
		static void init_source(j_decompress_ptr cinfo);
		static boolean fill_input_buffer(j_decompress_ptr cinfo);
		static void skip_input_data(j_decompress_ptr cinfo, long num_bytes);
		static void term_source(j_decompress_ptr cinfo);
		static void jpeg_mem_src(j_decompress_ptr cinfo, void* buffer, long nbytes);
	}
}
