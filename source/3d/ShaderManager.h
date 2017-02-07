#pragma once
#include "../ResourceManager.h"

namespace dv3d
{
	typedef uint32_t GLPROGHANDLE;

#define INVALID_GLPROGHANDLE 0

#define PROGRAM_STORAGE_SIZE 1024

	class ShaderManager
	{
		typedef std::pair<GLPROGHANDLE, GLuint> GLProgHandleAndReference;
		typedef std::unordered_map<GLPROGHANDLE, std::vector<GLuint>> PendingProgramShaders;
	private:
		packed_freelist<GLuint> _programs;
		resman::ResourceManager* _resManager;
		PendingProgramShaders _pendingProgramShaders;

	public:
		explicit ShaderManager(resman::ResourceManager* resManager);
		~ShaderManager();

		GLPROGHANDLE NewProgram();
		bool AttachAndCompileShader(GLPROGHANDLE handle, const resman::ResourceRequest &request);
		bool AttachAndCompileShader(GLPROGHANDLE handle, GLenum type, std::string &shaderSrc);
		bool LinkAndFinishProgram(GLPROGHANDLE handle);
		GLuint Get(const GLPROGHANDLE &handle) const;
		void Unload(const GLPROGHANDLE &handle);
		void UnloadAllOf(const std::vector<GLPROGHANDLE> &handles);
		void UnloadAll();
	};
}