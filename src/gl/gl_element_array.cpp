#include "gl_element_array.hpp"
#include "gl_debug.hpp"

ElementArray::ElementArray(unsigned int* data, unsigned int size, GLenum usage): m_size(size) {
    GLCALL(glGenBuffers(1, &m_renderer_id));
    GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id));
    GLCALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage));
    GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
};

ElementArray::~ElementArray() {
    GLCALL(glDeleteBuffers(1, &m_renderer_id));
};

void ElementArray::Use() {
    GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_renderer_id));
};

void ElementArray::UnUse() {
    GLCALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
};