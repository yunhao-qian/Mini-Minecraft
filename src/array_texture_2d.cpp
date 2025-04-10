#include "array_texture_2d.h"

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

    _context->glGenTextures(1, &_texture);
    _context->debugError();
    _context->glBindTexture(GL_TEXTURE_2D_ARRAY, _texture);
    _context->debugError();

    const auto tileWidth{image.width() / tileColumns};
    const auto tileHeight{image.height() / tileRows};

    // Allocate texture memory without specifying the actual data.
    const auto depth{tileRows * tileColumns};
    for (auto level{0}, scaledTileWidth{tileWidth}, scaledTileHeight{tileHeight};
         scaledTileWidth > 0 && scaledTileHeight > 0;
         ++level, scaledTileWidth /= 2, scaledTileHeight /= 2) {
        _context->glTexImage3D(GL_TEXTURE_2D_ARRAY,
                               level,
                               GL_RGBA8,
                               scaledTileWidth,
                               scaledTileHeight,
                               depth,
                               0,
                               GL_RGBA,
                               GL_UNSIGNED_BYTE,
                               nullptr);
        _context->debugError();
    }

    // Upload each tile to the texture memory.
    for (const auto tileY : std::views::iota(0, tileRows)) {
        for (const auto tileX : std::views::iota(0, tileColumns)) {
            const auto tileIndex{tileY * tileColumns + tileX};
            auto tileImage{image.copy(tileX * tileWidth, tileY * tileHeight, tileWidth, tileHeight)};
            // QImage has its origin at the top-left corner, while OpenGL textures have their
            // origins at the bottom-left corner.
            // https://doc.qt.io/qt-6/coordsys.html
            tileImage.flip(Qt::Vertical);
            _context->glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
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
            _context->debugError();
        }
    }

    _context->glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    _context->debugError();

    // Use nearest interpolation to create a more pixelated look.
    _context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    _context->debugError();
    _context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _context->debugError();

    // GL_REPEAT makes it possible to simulate water and lava animations by shifting the sampling
    // window.
    _context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    _context->debugError();
    _context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    _context->debugError();
}

} // namespace minecraft
