#ifndef MINI_MINECRAFT_ARRAY_TEXTURE_2D
#define MINI_MINECRAFT_ARRAY_TEXTURE_2D

#include "opengl_context.h"

#include <QString>

namespace minecraft {

class ArrayTexture2D
{
public:
    ArrayTexture2D(OpenGLContext *const context);
    ArrayTexture2D(const ArrayTexture2D &) = delete;
    ArrayTexture2D(ArrayTexture2D &&) = delete;

    ~ArrayTexture2D();

    ArrayTexture2D &operator=(const ArrayTexture2D &) = delete;
    ArrayTexture2D &operator=(ArrayTexture2D &&) = delete;

    void generate(const QString &fileName, const int tileRows, const int tileColumns);

    GLuint texture() const;

private:
    OpenGLContext *_context;
    GLuint _texture;
};

inline ArrayTexture2D::ArrayTexture2D(OpenGLContext *const context)
    : _context{context}
    , _texture{0u}
{}

inline ArrayTexture2D::~ArrayTexture2D()
{
    _context->glDeleteTextures(1, &_texture);
    _context->debugError();
}

inline GLuint ArrayTexture2D::texture() const
{
    return _texture;
}

} // namespace minecraft

#endif // MINI_MINECRAFT_ARRAY_TEXTURE_2D
