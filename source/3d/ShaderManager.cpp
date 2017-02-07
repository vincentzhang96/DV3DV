#include "../stdafx.h"
#include "ShaderManager.h"

dv3d::ShaderManager::ShaderManager(resman::ResourceManager* resManager) : _programs(PROGRAM_STORAGE_SIZE)
{
	assert(resManager != nullptr);
	_resManager = resManager;
}

dv3d::ShaderManager::~ShaderManager()
{
	UnloadAll();
}

dv3d::GLPROGHANDLE dv3d::ShaderManager::NewProgram()
{
	GLuint program = glCreateProgram();
	if (program == 0)
	{
		return INVALID_GLPROGHANDLE;
	}
	GLPROGHANDLE ret = _programs.insert(program);
	//	Add pending
	auto pair = _pendingProgramShaders.emplace(std::make_pair(ret, std::vector<GLuint>()));
	//	Clear if it already exists
	pair.first->second.clear();
	return ret;
}

bool dv3d::ShaderManager::AttachAndCompileShader(GLPROGHANDLE handle, const resman::ResourceRequest& request)
{
	if (request.type == resman::REQ_TPUID)
	{
		GLenum shdrType;
		switch(request.resTpuid.t)
		{
		case PPACT_TEXT_GLSL_VE_SHDR:
			shdrType = GL_VERTEX_SHADER;
			break;
		case PPACT_TEXT_GLSL_FR_SHDR:
			shdrType = GL_FRAGMENT_SHADER;
			break;
		case PPACT_TEXT_GLSL_GE_SHDR:
			shdrType = GL_GEOMETRY_SHADER;
			break;
		case PPACT_TEXT_GLSL_TC_SHDR:
			shdrType = GL_TESS_CONTROL_SHADER;
			break;
		case PPACT_TEXT_GLSL_TE_SHDR:
			shdrType = GL_TESS_EVALUATION_SHADER;
			break;
		default:
			LOG(WARNING) << "Unknown shader type " << request.resTpuid.t;
			return false;
		}
		auto data = _resManager->GetResource(request);
		if (!data._present)
		{
			LOG(WARNING) << "Unable to load data";
			return false;
		}
		auto shdrSrc = std::string(reinterpret_cast<char*>(data._data.data()), data._data.size());
		return AttachAndCompileShader(handle, shdrType, shdrSrc);
	}
	else
	{
		LOG(WARNING) << "Unsupported ResourceRequest for shader loading";
		return false;
	}
}

bool dv3d::ShaderManager::AttachAndCompileShader(GLPROGHANDLE handle, GLenum type, std::string &shaderSrc)
{
	GLuint prog;
	if (!_programs.contains(handle))
	{
		LOG(WARNING) << "Nonexistant shader program for handle " << handle;
		return false;
	}
	prog = _programs[handle];
	GLuint shdr = glCreateShader(type);
	auto shdrCstr = shaderSrc.c_str();
	glShaderSource(shdr, 1, &shdrCstr, nullptr);
	glCompileShader(shdr);
	GLint success;
	glGetShaderiv(shdr, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(shdr, 512, nullptr, infoLog);
		LOG(WARNING) << "Shader compilation failed: " << infoLog;
		return false;
	}
	glAttachShader(prog, shdr);
	//	Add this shader to the list
	_pendingProgramShaders[prog].push_back(prog);
	return true;
}

bool dv3d::ShaderManager::LinkAndFinishProgram(GLPROGHANDLE handle)
{
	auto prog = _programs[handle];
	glLinkProgram(prog);
	auto list = _pendingProgramShaders[prog];
	//	Delete the shaders because we don't need them anymore after linking
	for (auto shdr : list)
	{
		glDeleteShader(shdr);
	}
	//	Don't need the list anymore
	list.clear();
	//	Remove from pending
	_pendingProgramShaders.erase(prog);
	int success;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetProgramInfoLog(prog, 512, nullptr, infoLog);
		LOG(WARNING) << "Shader linking failed: " << infoLog;
		return;
	}
	return true;
}

GLuint dv3d::ShaderManager::Get(const GLPROGHANDLE& handle) const
{
	if (_programs.contains(handle))
	{
		return _programs[handle];
	}
	return 0;
}

void dv3d::ShaderManager::Unload(const GLPROGHANDLE& handle)
{
	if (_programs.contains(handle)) {
		auto prog = _programs[handle];
		glDeleteProgram(prog);
		_programs.erase(handle);
		LOG(DEBUG) << "Deleted texhandle " << handle << " GLTEXID " << prog;
	}
	else
	{
		LOG(WARNING) << "Attempted to unload a texture that doesn't exist or has already been unloaded: texhandle " << handle;
	}
}

void dv3d::ShaderManager::UnloadAllOf(const std::vector<GLPROGHANDLE>& handles)
{
	for (auto handle : handles)
	{
		Unload(handle);
	}
}

void dv3d::ShaderManager::UnloadAll()
{
	for (auto handle : _programs)
	{
		Unload(handle);
	}
}
