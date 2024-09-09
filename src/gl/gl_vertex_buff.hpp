#pragma once
#include "glad.h"
#include "gl_debug.hpp"

class VertexBuffer {
    public:
        VertexBuffer(GLenum type = GL_ARRAY_BUFFER);
        
        ~VertexBuffer();

        void SetData(const void* data, int size, GLenum data_type ,GLenum usage = GL_STATIC_DRAW);

        void SetSubData(const void *data, int size, int offset = -1);

        void Use();

        void UnUse();

        GLenum usage;
        GLenum buffer_type;
        GLenum data_type;
        int m_size;
        unsigned int m_render_id;
};