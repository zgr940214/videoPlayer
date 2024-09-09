#pragma once
#include "glad.h"
#include <vector>

class ElementAttribute {
    public: 
        ElementAttribute()=default;
        ~ElementAttribute(){};

        template<typename Ty>
        void Push(unsigned count) {
            static_assert(sizeof(Ty) == 0, "Unsupported type for Push");
        };

        template<>
        void Push<float> (unsigned count) {
            m_attributes.push_back({GL_FLOAT, count});
            stride += sizeof(float) * count;
        };

        template<>
        void Push<int> (unsigned count) {
            m_attributes.push_back({GL_INT, count});
            stride += sizeof(int) * count;
        }

        template<>
        void Push<char>(unsigned count) {
            m_attributes.push_back({GL_BYTE, count});
            stride += sizeof(char) * count;
        }

        void Clear(){
            m_attributes.clear();
            stride = 0;
        }
    
        std::vector<std::pair<GLenum, int>> m_attributes;
        unsigned stride = 0;
};

