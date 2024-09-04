#include "gl_vertex_buff.hpp"

VertexBuffer::VertexBuffer(GLenum type):buffer_type(type), m_size(0) {
    GLCALL(glGenBuffers(1, &m_render_id));
};

VertexBuffer::~VertexBuffer() {
    GLCALL(glDeleteBuffers(1, &m_render_id));
};

void VertexBuffer::SetData(const void* data, int size, GLenum data_type ,GLenum usage) {
    GLCALL(glBufferData(GL_ARRAY_BUFFER, size, data, usage));
    m_size = size;
    this->usage = usage;
    this->data_type = data_type;
}

void VertexBuffer::SetSubData(const void *data, int size, int offset) {
    if (offset == -1)
        offset = size;
    GLCALL(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
    if (m_size < size + offset) {
        m_size = size + offset;
    }
}

void VertexBuffer::Use() {
    GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_render_id));
}

void VertexBuffer::UnUse() {
    GLCALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}