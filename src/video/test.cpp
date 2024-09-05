#include "glm/glm.hpp"
#include "glad/glad.h"
#include "glfw3.h"
#include "video_common.h"

int OpenFile(const char *filename, AVFormatContext **fmt_ctx, 
    AVCodecContext **codec_ctx) {
        int ret;
        ret = avformat_open_input(fmt_ctx, filename, NULL, NULL);
        if (ret < 0) {
            fprintf(stderr, "failed to open input: %s\n", filename);
            exit(1);
        }

        ret = avformat_find_stream_info(*fmt_ctx, NULL);
        if (ret < 0) {
            fprintf(stderr, "failed to find stream info\n");
        }

        av_find_best_stream(*fmt_ctx, AVMEDIA_TYPE_VIDEO, );

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            // Add more key cases as needed
            default:
                break;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        // Handle left mouse button press
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // Handle cursor position changes
}


int main() {
    int xpos, ypos, width, height;
    const char *description;
    GLFWwindow *window = NULL;

    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed\n");
        exit(1);
    }

    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), 
        &xpos, &ypos, &width, &height);
    
    const int h = height / 5;
    const int w = width / 5;
    xpos += w * 2;
    ypos += h * 2;

    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GL_TRUE);
    glfwWindowHint(GLFW_POSITION_X, xpos);
    glfwWindowHint(GLFW_POSITION_Y, ypos);

    window = glfwCreateWindow(w, h, "videotest", NULL, NULL);
    if (!window) {
        glfwGetError(&description);
        fprintf(stderr, "glfwCreateWindow error: %s\n", description);
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "failed to load GL Loader\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        exit(1);
    }

     // Register the keyboard callback
    glfwSetKeyCallback(window, key_callback);

    // Register the mouse button callback
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Register the cursor position callback
    glfwSetCursorPosCallback(window, cursor_position_callback);

    glClearColor(1.f, 1.f, 1.f, 1.f);

    while(1) {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwPollEvents();

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}