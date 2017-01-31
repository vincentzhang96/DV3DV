#pragma once
#include "../ResourceManager.h"

namespace dv3d
{
	typedef uint32_t GLTEXHANDLE;

#define INVALID_GLTEXHANDLE 0

#define MAGIC_DDS 0x20534444



	struct GLTexture
	{
		GLuint _glTex;
		GLenum _texTarget;

		GLTexture();
		explicit GLTexture(GLenum target);
		~GLTexture();
	};

	struct GLTextureReference
	{
		GLuint _glTex;
		GLenum _texTarget;

		explicit GLTextureReference(const GLTexture &tex);
		explicit GLTextureReference(const GLuint &id);

		explicit operator bool() const;

		void Attach() const;
		void Detatch() const;
	};

	class TextureManager
	{
		typedef std::pair<GLTEXHANDLE, GLTextureReference> GLTexHandleAndReference;
	private:
		packed_freelist<GLTexture> _textures;
		resman::ResourceManager* _resManager;

	public:
		explicit TextureManager(resman::ResourceManager* resManager);
		~TextureManager();

		GLTEXHANDLE Load(const resman::ResourceRequest &request);
		GLTEXHANDLE LoadDDS(std::vector<uint8_t> &data);
		GLTexHandleAndReference LoadAndGet(const resman::ResourceRequest &request);
		GLTextureReference Get(const GLTEXHANDLE &handle) const;
		void Unload(const GLTEXHANDLE &handle);
		void UnloadAllOf(const std::vector<GLTEXHANDLE> &handles);
		void UnloadAll();
	};


}
