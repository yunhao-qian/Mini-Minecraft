#ifndef MINECRAFT_OPENGL_OBJECT_H
#define MINECRAFT_OPENGL_OBJECT_H

#include "opengl_context.h"

#include <QOpenGLFunctions_4_1_Core>

namespace minecraft {

class OpenGLObject
{
public:
    OpenGLObject()
        : _id{0u}
        , _deleter{nullptr}
    {}

    OpenGLObject(const GLuint id, void (*const deleter)(OpenGLContext *, GLuint))
        : _id{id}
        , _deleter{deleter}
    {}

    OpenGLObject(const OpenGLObject &) = delete;

    OpenGLObject(OpenGLObject &&other)
        : _id{other._id}
        , _deleter{other._deleter}
    {
        other._id = 0u;
        other._deleter = nullptr;
    }

    ~OpenGLObject() { reset(); }

    OpenGLObject &operator=(const OpenGLObject &) = delete;

    OpenGLObject &operator=(OpenGLObject &&other)
    {
        if (this != &other) {
            reset(other.release());
            _deleter = other._deleter;
            other._deleter = nullptr;
        }
        return *this;
    }

    GLuint release()
    {
        const auto id{_id};
        _id = 0u;
        return id;
    }

    void reset(const GLuint id = 0u)
    {
        if (_id != 0u && _deleter != nullptr) {
            const auto context{OpenGLContext::instance()};
            _deleter(context, _id);
            context->checkError();
        }
        _id = id;
    }

    GLuint get() const { return _id; }

    explicit operator bool() const { return _id != 0u; }

private:
    GLuint _id;
    void (*_deleter)(OpenGLContext *, GLuint);
};

} // namespace minecraft

#endif // MINECRAFT_OPENGL_OBJECT_H
