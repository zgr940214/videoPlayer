#pragma once
#include "gl_element_attribute.hpp"
#include "gl_debug.hpp"
#include <memory>

class ElementArray;
class VertexBuffer;

class VertexArray {
    public:
        VertexArray(); 

        ~VertexArray();

        void Use();

        void UnUse();

        void SetAttributeSeperated(ElementAttribute layout, std::vector<int> off);

        void SetAttribute(ElementAttribute layout);

        bool AddVbo(std::shared_ptr<VertexBuffer> vbo);

        bool AddEbo(std::shared_ptr<ElementArray> ebo);

        bool CheckAvailable(bool eb_available = false);

        void SetMode(GLenum mode);

        GLenum GetMode();

        void SetOff(unsigned int offset);

        unsigned int GetOff();

        unsigned int GetVboSize();

        unsigned int GetEboSize();

    private:
        unsigned m_render_id;
        GLenum m_mode;
        unsigned int m_offset;
        std::weak_ptr<ElementArray> m_current_ebo;
        std::weak_ptr<VertexBuffer> m_current_vbo;
};