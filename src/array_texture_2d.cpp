#include "array_texture_2d.h"

#include "opengl_context.h"

#include <QImage>

#include <ranges>

namespace minecraft {

void ArrayTexture2D::generate(const QString &fileName, const int tileRows, const int tileColumns)
{
    QImage image{fileName};
    if (image.isNull()) {
        qFatal() << "Failed to load texture image" << fileName;
    }
    image = image.convertToFormat(QImage::Format_RGBA8888);

    const auto context{OpenGLContext::instance()};

    {
        GLuint texture{0u};
        context->glGenTextures(1, &texture);
        context->checkError();
        _texture = OpenGLObject{
            texture,
            [](OpenGLContext *const context, const GLuint texture) {
                context->glDeleteTextures(1, &texture);
            },
        };
    }
    context->glBindTexture(GL_TEXTURE_2D_ARRAY, _texture.get());
    context->checkError();

    const auto tileWidth{image.width() / tileColumns};
    const auto tileHeight{image.height() / tileRows};

    // Allocate texture memory without specifying the actual data.
    const auto depth{tileRows * tileColumns};
    for (auto level{0}, scaledTileWidth{tileWidth}, scaledTileHeight{tileHeight};
         scaledTileWidth > 0 && scaledTileHeight > 0;
         ++level, scaledTileWidth /= 2, scaledTileHeight /= 2) {
        context->glTexImage3D(GL_TEXTURE_2D_ARRAY,
                              level,
                              GL_RGBA8,
                              scaledTileWidth,
                              scaledTileHeight,
                              depth,
                              0,
                              GL_RGBA,
                              GL_UNSIGNED_BYTE,
                              nullptr);
        context->checkError();
    }

    // Upload each tile to the texture memory.
    for (const auto tileY : std::views::iota(0, tileRows)) {
        for (const auto tileX : std::views::iota(0, tileColumns)) {
            const auto tileIndex{tileY * tileColumns + tileX};
            auto tileImage{image.copy(tileX * tileWidth, tileY * tileHeight, tileWidth, tileHeight)};
            // QImage has its origin at the top-left corner, while OpenGL textures have their
            // origins at the bottom-left corner.
            // https://doc.qt.io/qt-6/coordsys.html
            tileImage.mirror(false, true);
            context->glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                                     0,
                                     0,
                                     0,
                                     tileIndex,
                                     tileWidth,
                                     tileHeight,
                                     1,
                                     GL_RGBA,
                                     GL_UNSIGNED_BYTE,
                                     tileImage.constBits());
            context->checkError();
        }
    }

    context->glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    context->checkError();

    // Use nearest interpolation to create a more pixelated look.
    context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    context->checkError();
    context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    context->checkError();

    // GL_REPEAT makes it possible to simulate water and lava animations by shifting the sampling
    // window.
    context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    context->checkError();
    context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    context->checkError();
}

} // namespace minecraft
