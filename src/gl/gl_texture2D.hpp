#pragma once
#include "glad.h"
#include <string>

class Texture2D{
    public:
        Texture2D();

        ~Texture2D();

        void Generate(int w, int h, unsigned char* data);

        void Bind(GLenum index = GL_TEXTURE0);

        void UnBind();

    public:
        int width;
        int height;
        unsigned m_renderer_id;
        unsigned internal_format;
        unsigned image_format;
        unsigned wrap_s;
        unsigned wrap_t;
        unsigned filter_min;
        unsigned filter_max;
};