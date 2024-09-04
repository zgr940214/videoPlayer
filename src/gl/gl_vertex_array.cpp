#include "gl_vertex_array.hpp"
#include "gl_vertex_buff.hpp"
#include "gl_element_array.hpp"

#include <iostream>

VertexArray::VertexArray():m_mode(GL_TRIANGLES) { GLCALL(glGenVertexArrays(1, &m_render_id));};

VertexArray::~VertexArray() {GLCALL(glDeleteVertexArrays(1, &m_render_id));};

void VertexArray::Use() { GLCALL(glBindVertexArray(m_render_id));}

void VertexArray::UnUse() { GLCALL(glBindVertexArray(0));}

void VertexArray::SetAttributeSeperated(ElementAttribute layout, std::vector<int> off) {
    int index = 0;
    int start = 0;
    int stride = layout.stride;
    for (const auto &attribute : layout.m_attributes) {
        switch (attribute.first)
        {
        case GL_FLOAT: {
            start += off[index]; 
            GLCALL(glEnableVertexAttribArray(index));
            GLCALL(glVertexAttribPointer(index, attribute.second, GL_FLOAT, GL_FALSE,stride, reinterpret_cast<const void*>(start)));
            index++;
            break;
        }
        case GL_INT: {
            start += off[index]; 
            GLCALL(glEnableVertexAttribArray(index));
            GLCALL(glVertexAttribPointer(index, attribute.second, GL_INT, GL_FALSE,stride, reinterpret_cast<const void*>(start)));
            index++;
            break;
        }
        case GL_BYTE: {
            start += off[index]; 
            GLCALL(glEnableVertexAttribArray(index));
            GLCALL(glVertexAttribPointer(index, attribute.second, GL_BYTE, GL_FALSE, stride, reinterpret_cast<const void*>(start)));
            index++;
            break;
        }
        default:
            break;
        }
    }
}

void VertexArray::SetAttribute(ElementAttribute layout) {
    int index = 0;
    int start = 0;
    int stride = layout.stride;
    for (const auto &attribute : layout.m_attributes) {
        switch (attribute.first)
        {
        case GL_FLOAT: {
            GLCALL(glVertexAttribPointer(index, attribute.second, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(start)));
            GLCALL(glEnableVertexAttribArray(index++));
            start += sizeof(float) * attribute.second;
            break;
        }
        case GL_INT: {
            GLCALL(glVertexAttribPointer(index, attribute.second, GL_INT, GL_FALSE,stride, reinterpret_cast<void*>(start)));
            GLCALL(glEnableVertexAttribArray(index++));
            start += sizeof(int) * attribute.second;
            break;
        }
        case GL_BYTE: {
            GLCALL(glVertexAttribPointer(index++, attribute.second, GL_BYTE, GL_FALSE, stride, reinterpret_cast<void*>(start)));
            GLCALL(glEnableVertexAttribArray(index));
            start += sizeof(char) * attribute.second;
            break;
        }
        default:
            break;
        }
    }
}

void VertexArray::SetMode(GLenum mode) {
    m_mode = mode;
};

bool VertexArray::AddVbo(std::shared_ptr<VertexBuffer> vbo) {
    m_current_vbo = vbo;
    if (vbo) {
        Use();
        vbo->Use();
        UnUse();
        vbo->UnUse();

        return true;
    }
    return false;
};

bool VertexArray::AddEbo(std::shared_ptr<ElementArray> ebo) {
    m_current_ebo = ebo;
    if (ebo) {
        Use();
        ebo->Use();
        UnUse();
        return true;
    }
    return false;
};

bool VertexArray::CheckAvailable(bool eb_available) {
    bool ret = false;
    auto eb = m_current_ebo.lock();
    auto vb = m_current_vbo.lock();
    if (eb_available) {
        if (eb && vb) {
            ret = true;
        }
    } else if (vb) {
        ret = true;
    } 
    return ret;
};

GLenum VertexArray::GetMode() {
    return m_mode;
};

void VertexArray::SetOff(unsigned int offset) {
    m_offset = offset;
};

unsigned int VertexArray::GetOff() {
    return m_offset;
};

unsigned int VertexArray::GetVboSize() {
    auto vb = m_current_vbo.lock();
    if (vb) {
        return vb->m_size;
    }
    return 0;
};

unsigned int VertexArray::GetEboSize() {
    auto eb = m_current_ebo.lock();
    if (eb) {
        return eb->m_size;
    }
};