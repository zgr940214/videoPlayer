#pragma once
#include "glad.h"
#include <stdio.h>

#define ASSERT(x) if(!x)\
                    __debugbreak();

#define GLCALL(x) GLClearError();\
                  x;\
                  ASSERT(GLPrintError(__FILE__, __LINE__));


static void GLClearError() {
    GLenum err;
    while(err = glGetError() != GL_NO_ERROR);
};

static bool GLPrintError(const char* file, int line) {
    bool ret = true;
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) {
        ret = false;
        printf("[opengl error: %d], file:%s, line:%d", err, file, line);
    }
    return ret;
}