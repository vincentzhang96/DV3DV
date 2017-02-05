#pragma once
#include "../ResourceManager.h"

namespace dv3d
{
	typedef uint32_t GLTEXHANDLE;

#define INVALID_GLTEXHANDLE 0

#define MAGIC_DDS 0x20534444


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
		GLTexHandleAndReference LoadAndGet(const resman::ResourceRequest &request);
		GLuint Get(const GLTEXHANDLE &handle) const;
		void Unload(const GLTEXHANDLE &handle);
		void UnloadAllOf(const std::vector<GLTEXHANDLE> &handles);
		void UnloadAll();
	};


}
