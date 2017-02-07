#include "../stdafx.h"
#include "ShaderManager.h"

dv3d::PendingProgram::PendingProgram()
{
	success = true;
}

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
	auto pair = _pendingProgramShaders.emplace(ret, PendingProgram());
	//	Handles are unique so it can't have previously existed
	return ret;
}

void dv3d::ShaderManager::AttachAndCompileShader(const GLPROGHANDLE &handle, const resman::ResourceRequest& request)
{
	if (!_programs.contains(handle))
	{
		LOG(WARNING) << "Nonexistant shader program for handle " << handle;
		return;
	}
	auto pending = _pendingProgramShaders[handle];
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
			pending.success = false;
			return;
		}
		auto data = _resManager->GetResource(request);
		if (!data._present)
		{
			LOG(WARNING) << "Unable to load data";
			pending.success = false;
			return;
		}
		auto shdrSrc = std::string(reinterpret_cast<char*>(data._data.data()), data._data.size());
		AttachAndCompileShader(handle, shdrType, shdrSrc);
	}
	else
	{
		LOG(WARNING) << "Unsupported ResourceRequest for shader loading";
		pending.success = false;
		return;
	}
}

void dv3d::ShaderManager::AttachAndCompileShader(const GLPROGHANDLE &handle, GLenum type, std::string &shaderSrc)
{
	assert(_programs.contains(handle));
	PendingProgramShaders::mapped_type pendingProgram = _pendingProgramShaders[handle];
	GLuint prog = _programs[handle];
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
		pendingProgram.success = false;
		return;
	}
	glAttachShader(prog, shdr);
	//	Add this shader to the list
	pendingProgram.pendingShaders.push_back(shdr);
	return;
}

bool dv3d::ShaderManager::LinkAndFinishProgram(const GLPROGHANDLE &handle)
{
	auto prog = _programs[handle];
	glLinkProgram(prog);
	auto list = _pendingProgramShaders.at(handle).pendingShaders;
	//	Delete the shaders because we don't need them anymore after linking
	for (auto shdr : list)
	{
		glDeleteShader(shdr);
	}
	//	Don't need the list anymore
	list.clear();
	//	Remove from pending
	bool pendingOk = _pendingProgramShaders.at(handle).success;
	_pendingProgramShaders.erase(handle);
	int success;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetProgramInfoLog(prog, 512, nullptr, infoLog);
		LOG(WARNING) << "Shader linking failed: " << infoLog;
		return false;
	}
	LOG(DEBUG) << "Successfully linked shader program with proghandle " << handle << " GLPROGID " << prog;
	return pendingOk;
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
		LOG(DEBUG) << "Deleted proghandle " << handle << " GLPROGID " << prog;
	}
	else
	{
		LOG(WARNING) << "Attempted to unload a shader program that doesn't exist or has already been unloaded: proghandle " << handle;
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
	size_t len = _programs.size();
	for (auto handle : _programs)
	{
		Unload(handle);
	}
	LOG(DEBUG) << "Cleared all " << len << " shader programs";
}
