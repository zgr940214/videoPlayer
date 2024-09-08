#include "glm/glm.hpp"
#include "glad/glad.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "glfw3.h"
#include "video_common.h"
#include "video_codec.h"
#include "memory_pool.h"
#include "clock.h"

#include <vector>
#include <mutex>

clock_t *clock = NULL;
std::atomic_int should_update = 0;

int OpenFile(const char *filename, AVFormatContext **fmt_ctx, 
    AVCodecContext **codec_ctx) {
        int ret;
        const AVCodec *codec;
        ret = avformat_open_input(fmt_ctx, filename, NULL, NULL);
        if (ret < 0) {
            fprintf(stderr, "failed to open input: %s\n", filename);
            exit(1);
        }

        ret = avformat_find_stream_info(*fmt_ctx, NULL);
        if (ret < 0) {
            fprintf(stderr, "failed to find stream info\n");
            exit(1);
        }

        int vstream_id = av_find_best_stream(*fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
        if (vstream_id == AVERROR_STREAM_NOT_FOUND) {
            fprintf(stderr, "can not find video stream id");
            exit(1);
        }

        codec = avcodec_find_decoder((*fmt_ctx)->streams[vstream_id]->codecpar->codec_id);
        if (!codec) {
            fprintf(stderr, "can not find decoder\n");
            exit(1);
        }

        *codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            fprintf(stderr, "cant alloc codec context\n");
            exit(1);
        }

        if (ret = avcodec_parameters_to_context(*codec_ctx, (*fmt_ctx)->streams[vstream_id]->codecpar) < 0) {
            fprintf(stderr, "avcodec_parameters_to_context failed\n");
            exit(1);
        }

        if ((ret = avcodec_open2(*codec_ctx, codec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
            return ret;
        }

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

void decoder_thread(AVFormatContext *fmt_ctx, std::vector<AVFrame*> &queue, std::mutex &mtx) {
    AVPacket *pkt = NULL;
    pkt = av_packet_alloc();
    ret = 0;
    while((ret = av_read_frame(fmt_ctx, pkt)) >= 0) {
        int ret = 0;
        AVFrame *frame = av_frame_alloc();
        // 把AVPacket 送进 解码器
        ret = avcodec_send_packet(codec_ctx, pkt);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE]{0};
            av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
            fprintf(stderr, "Error submitting a packet for decoding (%s)\n", errbuf);
            return -1;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(codec_ctx, frame);
            if (ret < 0) {
                return ret;
            }
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                std::unique_lock<std::mutex> lk(mtx);
                queue.push_back(frame);
            }
        }
        av_packet_unref(pkt);
        // update timer
        tick_clock(clock);
        int64_t pts = transform_clock_to_video_pts(clock, frame->time_base.den, 30);
        {
            std::unique_lock<std::mutex> lk(mtx);
            if (!queue.empty() && 
                queue[0].pts <= pts) {
                should_update = 1;
            }
        }
    }
}

std::vector<AVFrame *> video_queue;
std::mutex mtx;

AVFrame* vframe_to_rgb(AVFrame *frame) {
    // 假设avFrame是一个有效的AVFrame，已经包含了解码后的视频帧数据
    AVFrame* rgbFrame;
    static struct SwsContext* sws_ctx = NULL;

    int videoWidth = frame->width;
    int videoHeight = frame->height;

    // 为RGB帧分配内存
    rgbFrame = av_frame_alloc();
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoWidth, videoHeight, 32);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

    // 设置RGB帧的数据和格式
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, videoWidth, videoHeight, 32);

    // 创建格式转换上下文
    if (sws_ctx == NULL) {
        sws_ctx = sws_getContext(videoWidth, videoHeight, frame->format,
                                videoWidth, videoHeight, AV_PIX_FMT_RGB24,
                                SWS_BILINEAR, NULL, NULL, NULL);
    }

    // 转换颜色格式
    sws_scale(sws_ctx, (uint8_t const* const*)frame->data,
            frame->linesize, 0, videoHeight,
            rgbFrame->data, rgbFrame->linesize);
    return rgbFrame;
};

void frame_to_texture(AVFrame * frame) {
    // 在OpenGL中创建纹理
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->data[0]);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}



int main() {
    int xpos, ypos, width, height;
    const char *description;
    GLFWwindow *window = NULL;

    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed\n");
        exit(1);
    }

    create_clock(clock, clock_t::TIMEUNIT::MILLISECONDS, nullptr);

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
        if (should_update) {

        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}