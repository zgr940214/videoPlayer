#include "gl_texture2D.hpp"
#include "gl_debug.hpp"

#include <iostream>

    Texture2D::Texture2D(): width(0), height(0), m_renderer_id(-1), wrap_s(GL_REPEAT), wrap_t(GL_REPEAT),
        filter_min(GL_LINEAR), filter_max(GL_LINEAR), internal_format(GL_RGB), image_format(GL_RGB){};

    Texture2D::~Texture2D() {glDeleteTextures(1, &m_renderer_id);
        std::cout << "destory texture\n";
    }

    void Texture2D::Generate(int w, int h, unsigned char* data) {
        GLCALL(glGenTextures(1, &m_renderer_id));

        this->height = h;
        this->width = w;

        GLCALL(glBindTexture(GL_TEXTURE_2D ,m_renderer_id));
        GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, image_format, GL_UNSIGNED_BYTE, data));

        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_min));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_max));

        GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
    }

    void Texture2D::Bind(GLenum index) {
        GLCALL(glActiveTexture(index));
        GLCALL(glBindTexture(GL_TEXTURE_2D, m_renderer_id));
    }


    void Texture2D::UnBind() {
        GLCALL(glBindTexture(GL_TEXTURE_2D, 0));
    }