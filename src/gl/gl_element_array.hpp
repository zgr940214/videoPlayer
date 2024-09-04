#pragma once
#include "glad/glad.h"

class ElementArray {
    public:
        unsigned int m_renderer_id;
        unsigned int m_size;
    
    public:
        ElementArray(unsigned int* data, unsigned int size, GLenum usage = GL_STATIC_DRAW);
        ~ElementArray();

        void Use();
        void UnUse();
};