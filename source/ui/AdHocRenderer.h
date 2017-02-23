#pragma once

namespace dv3d
{
	namespace adhoc
	{

		class Renderer
		{
			const size_t STRIDE = 48;
			const size_t STRIDE_BYTES = sizeof(GLfloat) * STRIDE;
			const void* VERT_OFFSET = GLBUFFEROFFSET_F(0);
			const void* COLOR_OFFSET = GLBUFFEROFFSET_F(3);
			const void* NORMAL_OFFSET = GLBUFFEROFFSET_F(7);
			const void* TEXCOORD_OFFSET = GLBUFFEROFFSET_F(10);
			const size_t _maxVerts;

			GLuint _vao;
			GLuint _vbo;

			bool _drawStarted;
			size_t _numVerts;
			GLenum _drawingMode;
			GLfloat* _data;
			GLfloat* _dataIter;
			glm::fvec3 _normal;
			glm::fvec4 _color;
			glm::fvec2 _texCoords;


			void _PushBack(GLfloat f);

			void _AddVert(
				GLfloat x, GLfloat y, GLfloat z
			);

		public:
			size_t _totalPolysDrawnThisFrame;
			size_t _drawCalls;

			explicit Renderer(size_t maxVerts);
			~Renderer();

			void PostRendererInit();

			void BeginDraw(GLenum drawMode);

			void SetColorf(GLfloat r, GLfloat g, GLfloat b, GLfloat a = 1.0F);
			void SetColori(uint32_t color);
			void SetOpaqueColori(uint32_t color);
			void SetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);
			void SetColorfv4(glm::fvec4 color);
			void SetOpaqueColorfv3(glm::fvec3 color);

			void SetNormalf(GLfloat nX, GLfloat nY, GLfloat nZ);
			void SetNormalfv(glm::fvec3 normal);

			void SetTexCoordsf(GLfloat u, GLfloat v);
			void SetTexCoordsfv(glm::fvec2 texCoords);

			void AddVertexf(GLfloat x, GLfloat y, GLfloat z = 0.0F);
			void AddVertexfv2(glm::fvec2 vert);
			void AddVertexfv3(glm::fvec3 vert);

			void AddVertexColorf(GLfloat x, GLfloat y, GLfloat z, GLfloat r, GLfloat g, GLfloat b, GLfloat a = 1.0F);
			void AddVertexColorfv(glm::fvec3 vert, glm::fvec4 color);
			void AddVertexOpaqueColorfv(glm::fvec3 vert, glm::fvec3 color);
			
			void AddVertexNormalf(GLfloat x, GLfloat y, GLfloat z, GLfloat nX, GLfloat nY, GLfloat nZ);
			void AddVertexNormalfv(glm::fvec3 vert, glm::fvec3 norm);

			void AddVertexTexCoordf(GLfloat x, GLfloat y, GLfloat z, GLfloat u, GLfloat v);
			void AddVertexTexCoordfv(glm::fvec3 vert, glm::fvec2 texCoords);

			void AddVertexColorNormTexCoordf(
				GLfloat x, GLfloat y, GLfloat z,
				GLfloat r, GLfloat g, GLfloat b, GLfloat a,
				GLfloat nX, GLfloat nY, GLfloat nZ,
				GLfloat u, GLfloat v
			);
			void AddVertexColorNormTexCoordfv(glm::fvec3 vert, glm::fvec4 color, glm::fvec3 norm, glm::fvec2 texCoords);

			void EndDraw();

			void FinishFrame();

		};

	}


}
