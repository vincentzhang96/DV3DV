#include "../stdafx.h"
#include "AdHocRenderer.h"

void dv3d::adhoc::Renderer::_PushBack(GLfloat f)
{
	*_dataIter = f;
	++_dataIter;
}

void dv3d::adhoc::Renderer::_AddVert(GLfloat x, GLfloat y, GLfloat z)
{
	if (!_drawStarted)
	{
		LOG(WARNING) << "Attempted to add a vert when not drawing";
		return;
	}
	if (_numVerts >= _maxVerts)
	{
		LOG(WARNING) << "Too many verts";
		return;
	}
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

dv3d::adhoc::Renderer::Renderer(size_t maxVerts) :
	_maxVerts(maxVerts)
{
	_data = new GLfloat[maxVerts * (3 + 4 + 3 + 2)];
	_dataIter = _data;
	_vao = 0;
	_vbo = 0;
	_drawStarted = false;
	_drawingMode = 0;
	_numVerts = 0;
	_totalPolysDrawnThisFrame = 0;
	_drawCalls = 0;
}

dv3d::adhoc::Renderer::~Renderer()
{
	glDeleteBuffers(1, &_vbo);
	glDeleteVertexArrays(1, &_vao);
	delete[] _data;
}

void dv3d::adhoc::Renderer::PostRendererInit()
{
	//	Set up VAO
	glGenVertexArrays(1, &_vao);
	glGenBuffers(1, &_vbo);
	glBindVertexArray(_vao);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	//	VBO STRUCTURE
	//	ID	OFF	SZ	TYPE	DESC
	//	0	0	4	FLOAT	x
	//	1	4	4	FLOAT	y
	//	2	8	4	FLOAT	z
	//	3	12	4	FLOAT	r
	//	4	16	4	FLOAT	g
	//	5	20	4	FLOAT	b
	//	6	24	4	FLOAT	a
	//	7	28	4	FLOAT	nX
	//	8	32	4	FLOAT	nY
	//	9	36	4	FLOAT	nZ
	//	10	40	4	FLOAT	u
	//	11	44	4	FLOAT	v
	glBufferData(GL_ARRAY_BUFFER, STRIDE_BYTES * 4, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);	//	Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE_BYTES, VERT_OFFSET);
	glEnableVertexAttribArray(1);	//	Color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, STRIDE_BYTES, COLOR_OFFSET);
	glEnableVertexAttribArray(2);	//	Normal
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, STRIDE_BYTES, NORMAL_OFFSET);
	glEnableVertexAttribArray(3);	//	TexCoord
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, STRIDE_BYTES, TEXCOORD_OFFSET);
	glBindVertexArray(0);
}

void dv3d::adhoc::Renderer::BeginDraw(GLenum drawMode)
{
	if (_drawStarted)
	{
		LOG(WARNING) << "Attempting to begin drawcall without previous drawcall finished";
		return;
	}
	//	Validate out drawing mode
	switch(drawMode)
	{
	case GL_TRIANGLES:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLE_STRIP:
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
	case GL_POINTS:
		break;
	default:
		LOG(WARNING) << "Unsupported drawing mode " << drawMode;
		return;
	}
	_drawingMode = drawMode;
	_drawStarted = true;
	_numVerts = 0;
	_dataIter = _data;
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
	if (_numVerts > 0)
	{
		glBindVertexArray(_vao);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, _numVerts * STRIDE_BYTES, _data, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);	//	Position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE_BYTES, VERT_OFFSET);
		glEnableVertexAttribArray(1);	//	Color
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, STRIDE_BYTES, COLOR_OFFSET);
		glEnableVertexAttribArray(2);	//	Normal
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, STRIDE_BYTES, NORMAL_OFFSET);
		glEnableVertexAttribArray(3);	//	TexCoord
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, STRIDE_BYTES, TEXCOORD_OFFSET);
		glDrawArrays(_drawingMode, 0, _numVerts);
		glBindVertexArray(0);
		switch (_drawingMode)
		{
		case GL_TRIANGLES:
			_totalPolysDrawnThisFrame += _numVerts / 3;
			break;
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLE_FAN:
			_totalPolysDrawnThisFrame += _numVerts - 2;
			break;
		case GL_LINES:
			_totalPolysDrawnThisFrame += _numVerts / 2;
			break;
		case GL_LINE_STRIP:
			_totalPolysDrawnThisFrame += _numVerts - 1;
			break;
		case GL_LINE_LOOP:
		case GL_POINTS:
			_totalPolysDrawnThisFrame += _numVerts;
			break;
		default:
			break;
		}
		++_drawCalls;
	}
	_drawStarted = false;
	_drawingMode = 0;
	_numVerts = 0;
}

void dv3d::adhoc::Renderer::FinishFrame()
{
	_totalPolysDrawnThisFrame = 0;
	_drawCalls = 0;
}







