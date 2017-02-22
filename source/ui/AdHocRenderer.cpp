#include "../stdafx.h"
#include "AdHocRenderer.h"

void dv3d::adhoc::Renderer::_PushBack(GLfloat f)
{
	*_dataIter = f;
	++_dataIter;
}

void dv3d::adhoc::Renderer::_AddVert(GLfloat x, GLfloat y, GLfloat z)
{
	//	VBO STRUCTURE
	//	OFF	SZ	TYPE	DESC
	//	0	4	FLOAT	x
	//	4	4	FLOAT	y
	//	8	4	FLOAT	z
	//	12	4	FLOAT	r
	//	16	4	FLOAT	g
	//	20	4	FLOAT	b
	//	24	4	FLOAT	a
	//	28	4	FLOAT	nX
	//	32	4	FLOAT	nY
	//	36	4	FLOAT	nZ
	//	40	4	FLOAT	u
	//	44	4	FLOAT	v
	_PushBack(x);
	_PushBack(y);
	_PushBack(z);
	_PushBack(_color.x);
	_PushBack(_color.y);
	_PushBack(_color.z);
	_PushBack(_color.w);
	_PushBack(_normal.x);
	_PushBack(_normal.y);
	_PushBack(_normal.z);
	_PushBack(_texCoords.x);
	_PushBack(_texCoords.y);
	++_numVerts;
}

void dv3d::adhoc::Renderer::BeginDraw(GLenum drawMode)
{
	if (_drawStarted)
	{
		LOG(WARNING) << "Attempting to begin drawcall without previous drawcall finished";
		throw "Already drawing";
	}
	//	Validate out drawing mode
	switch(drawMode)
	{
	case GL_TRIANGLES:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLE_STRIP:
	case GL_LINES:
	case GL_POINTS:
		break;
	default:
		LOG(WARNING) << "Unsupported drawing mode";
		throw "Unsupported drawMode";
	}
	_drawingMode = drawMode;
	_drawStarted = true;
	_numVerts = 0;
	_data.clear();
	_dataIter = _data.begin();
}

void dv3d::adhoc::Renderer::SetColorf(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	_color.x = r;
	_color.y = g;
	_color.z = b;
	_color.w = a;
}

void dv3d::adhoc::Renderer::SetColori(uint32_t color)
{
	GLfloat red = GLfloat((color >> 16) & 0xFF) / 255.0F;
	GLfloat green = GLfloat((color >> 8) & 0xFF) / 255.0F;
	GLfloat blue = GLfloat(color & 0xFF) / 255.0F;
	GLfloat alpha = GLfloat((color >> 24) & 0xFF) / 255.0F;
	SetColorf(red, green, blue, alpha);
}

void dv3d::adhoc::Renderer::SetOpaqueColori(uint32_t color)
{
	SetColori(0xFF000000 | color);
}

void dv3d::adhoc::Renderer::SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	SetColorf(r / 255.0F, g / 255.0F, b / 255.0F, a / 255.0F);
}

void dv3d::adhoc::Renderer::SetColorfv4(glm::fvec4 color)
{
	_color = color;
}

void dv3d::adhoc::Renderer::SetOpaqueColorfv3(glm::fvec3 color)
{
	SetColorf(color.x, color.y, color.z);
}

void dv3d::adhoc::Renderer::SetNormalf(GLfloat nX, GLfloat nY, GLfloat nZ)
{
	_normal.x = nX;
	_normal.y = nY;
	_normal.z = nZ;
}

void dv3d::adhoc::Renderer::SetNormalfv(glm::fvec3 normal)
{
	_normal = normal;
}

void dv3d::adhoc::Renderer::SetTexCoordsf(GLfloat u, GLfloat v)
{
	_texCoords.x = u;
	_texCoords.y = v;
}

void dv3d::adhoc::Renderer::SetTexCoordsfv(glm::fvec2 texCoords)
{
	_texCoords = texCoords;
}

void dv3d::adhoc::Renderer::AddVertexf(GLfloat x, GLfloat y, GLfloat z)
{
	AddVertexColorNormTexCoordf(
		x, y, z,
		_color.x, _color.y, _color.z, _color.w,
		_normal.x, _normal.y, _normal.z,
		_texCoords.x, _texCoords.y
	);
}

void dv3d::adhoc::Renderer::AddVertexfv2(glm::fvec2 vert)
{
	AddVertexf(vert.x, vert.y);
}

void dv3d::adhoc::Renderer::AddVertexfv3(glm::fvec3 vert)
{
	AddVertexColorNormTexCoordfv(
		vert,
		_color,
		_normal,
		_texCoords
	);
}

void dv3d::adhoc::Renderer::AddVertexColorf(GLfloat x, GLfloat y, GLfloat z, GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	SetColorf(r, g, b, a);
	AddVertexf(x, y, z);
}

void dv3d::adhoc::Renderer::AddVertexColorfv(glm::fvec3 vert, glm::fvec4 color)
{
	SetColorfv4(color);
	AddVertexfv3(vert);
}

void dv3d::adhoc::Renderer::AddVertexOpaqueColorfv(glm::fvec3 vert, glm::fvec3 color)
{
	SetOpaqueColorfv3(color);
	AddVertexfv3(vert);
}

void dv3d::adhoc::Renderer::AddVertexNormalf(GLfloat x, GLfloat y, GLfloat z, GLfloat nX, GLfloat nY, GLfloat nZ)
{
	SetNormalf(nX, nY, nZ);
	AddVertexf(x, y, z);
}

void dv3d::adhoc::Renderer::AddVertexNormalfv(glm::fvec3 vert, glm::fvec3 norm)
{
	SetNormalfv(norm);
	AddVertexfv3(vert);
}

void dv3d::adhoc::Renderer::AddVertexTexCoordf(GLfloat x, GLfloat y, GLfloat z, GLfloat u, GLfloat v)
{
	SetTexCoordsf(u, v);
	AddVertexf(x, y, z);
}

void dv3d::adhoc::Renderer::AddVertexTexCoordfv(glm::fvec3 vert, glm::fvec2 texCoords)
{
	SetTexCoordsfv(texCoords);
	AddVertexfv3(vert);
}

void dv3d::adhoc::Renderer::AddVertexColorNormTexCoordf(GLfloat x, GLfloat y, GLfloat z, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nX, GLfloat nY, GLfloat nZ, GLfloat u, GLfloat v)
{
	SetColorf(r, g, b, a);
	SetNormalf(nX, nY, nZ);
	SetTexCoordsf(u, v);
	_AddVert(x, y, z);
}

void dv3d::adhoc::Renderer::AddVertexColorNormTexCoordfv(glm::fvec3 vert, glm::fvec4 color, glm::fvec3 norm, glm::fvec2 texCoords)
{
	SetColorfv4(color);
	SetNormalfv(norm);
	SetTexCoordsfv(texCoords);
	_AddVert(vert.x, vert.y, vert.z);
}

void dv3d::adhoc::Renderer::EndDraw()
{

	glBindVertexArray(_vao);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, _numVerts * 48 * sizeof(GLfloat), _data.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDrawArrays(_drawingMode, 0, _numVerts);
}







