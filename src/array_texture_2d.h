#ifndef MINECRAFT_ARRAY_TEXTURE_2D
#define MINECRAFT_ARRAY_TEXTURE_2D

#include "opengl_object.h"

#include <QString>

namespace minecraft {

class ArrayTexture2D
{
public:
    ArrayTexture2D() = default;
    ArrayTexture2D(const ArrayTexture2D &) = delete;
    ArrayTexture2D(ArrayTexture2D &&) = delete;

    ArrayTexture2D &operator=(const ArrayTexture2D &) = delete;
    ArrayTexture2D &operator=(ArrayTexture2D &&) = delete;

    void generate(const QString &fileName, const int tileRows, const int tileColumns);

    GLuint texture() const { return _texture.get(); }

private:
    OpenGLObject _texture;
};

} // namespace minecraft

#endif // MINECRAFT_ARRAY_TEXTURE_2D
