#include "Core/Type.h"

#include <glad/gl.h>

#include <cstdio>
#include <cstdlib>
namespace cge
{

inline Char8_t const* fromGLErrorCode(GLenum err)
{
    switch (err)
    {
    case GL_NO_ERROR:
        return "No error";
    case GL_INVALID_ENUM:
        return "Invalid enum";
    case GL_INVALID_VALUE:
        return "Invalid value";
    case GL_INVALID_OPERATION:
        return "Invalid operation";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "Invalid framebuffer operation";
    case GL_OUT_OF_MEMORY:
        return "Out of memory";
    case GL_STACK_UNDERFLOW:
        return "Stack underflow";
    case GL_STACK_OVERFLOW:
        return "Stack overflow";
    default:
        return "Unknown error";
    }
}

inline void
  CheckOpenGLError(const Char8_t* stmt, const Char8_t* fname, I32_t line)
{
    GLenum err;
    B8_t   inError = false;
    do {
        err = glGetError();
        if (err != GL_NO_ERROR)
        {
            inError = true;
            std::printf(
              "OpenGL error %s, at %s:%i - for %s\n",
              fromGLErrorCode(err),
              fname,
              line,
              stmt);
        }
    } while (err != GL_NO_ERROR);
    if (inError) { std::abort(); }
}

#ifdef CGE_DEBUG
#define GL_CHECK(stmt)                                      \
    do {                                                    \
        stmt;                                               \
        ::cge::CheckOpenGLError(#stmt, __FILE__, __LINE__); \
    } while (0)
#else
#define GL_CHECK(stmt) stmt
#endif

inline U32_t typeSize(GLenum type)
{
    U32_t size = 0;
#define CASE(Enum, Count, Type)             \
    case Enum:                              \
        size = Count * (U32_t)sizeof(Type); \
        break
    switch (type)
    {
        CASE(GL_FLOAT, 1, GLfloat);
        CASE(GL_FLOAT_VEC2, 2, GLfloat);
        CASE(GL_FLOAT_VEC3, 3, GLfloat);
        CASE(GL_FLOAT_VEC4, 4, GLfloat);
        CASE(GL_INT, 1, GLint);
        CASE(GL_INT_VEC2, 2, GLint);
        CASE(GL_INT_VEC3, 3, GLint);
        CASE(GL_INT_VEC4, 4, GLint);
        CASE(GL_UNSIGNED_INT, 1, GLuint);
        CASE(GL_UNSIGNED_INT_VEC2, 2, GLuint);
        CASE(GL_UNSIGNED_INT_VEC3, 3, GLuint);
        CASE(GL_UNSIGNED_INT_VEC4, 4, GLuint);
        CASE(GL_BOOL, 1, GLboolean);
        CASE(GL_BOOL_VEC2, 2, GLboolean);
        CASE(GL_BOOL_VEC3, 3, GLboolean);
        CASE(GL_BOOL_VEC4, 4, GLboolean);
        CASE(GL_FLOAT_MAT2, 4, GLfloat);
        CASE(GL_FLOAT_MAT2x4, 8, GLfloat);
        CASE(GL_FLOAT_MAT3, 9, GLfloat);
        CASE(GL_FLOAT_MAT3x2, 6, GLfloat);
        CASE(GL_FLOAT_MAT3x4, 12, GLfloat);
        CASE(GL_FLOAT_MAT4, 16, GLfloat);
        CASE(GL_FLOAT_MAT4x2, 8, GLfloat);
        CASE(GL_FLOAT_MAT4x3, 12, GLfloat);
#undef CASE
    default:
        break;
    }
    return size;
}

} // namespace cge