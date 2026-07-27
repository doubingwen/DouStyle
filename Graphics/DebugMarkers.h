#pragma once

#include <GL/glew.h>

namespace DebugMarkers {

inline bool Available()
{
    return (GLEW_VERSION_4_3 || GLEW_KHR_debug)
        && glPushDebugGroup != nullptr
        && glPopDebugGroup != nullptr
        && glObjectLabel != nullptr;
}

inline void Push(const char* label)
{
    if (Available()) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, label);
    }
}

inline void Pop()
{
    if (Available()) {
        glPopDebugGroup();
    }
}

inline void Label(GLenum identifier, GLuint object, const char* label)
{
    if (Available() && object != 0) {
        glObjectLabel(identifier, object, -1, label);
    }
}

}
