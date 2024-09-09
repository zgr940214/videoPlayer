#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include "glad.h"
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#define GLFW_INCLUDE_NONE
#include "glfw3.h"
#include "video_common.h"
#include "video_codec.h"
#include "memory_pool.h"
#include "clock.h"

#include "gl_vertex_array.hpp"
#include "gl_vertex_buff.hpp"
#include "gl_shader.hpp"
#include "gl_element_attribute.hpp"

#include <vector>
#include <mutex>
#include <thread>
#include <functional>

sys_clock_t *ck = NULL;
std::atomic_int should_update = 0;
AVFormatContext *fmt_ctx = NULL;
AVCodecContext  *codec_ctx = NULL;
std::vector<AVFrame *> video_queue;
AVRational time_base;
std::mutex mtx;

int vstream_id = -1;

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

        vstream_id = av_find_best_stream(*fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (vstream_id == AVERROR_STREAM_NOT_FOUND) {
            fprintf(stderr, "can not find video stream id");
            exit(1);
        }

        time_base = (*fmt_ctx)->streams[vstream_id]->time_base;

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
        sws_ctx = sws_getContext(videoWidth, videoHeight, (AVPixelFormat)frame->format,
                                videoWidth, videoHeight, AV_PIX_FMT_RGB24,
                                SWS_BILINEAR, NULL, NULL, NULL);
    }

    // 转换颜色格式
    sws_scale(sws_ctx, (uint8_t const* const*)frame->data,
            frame->linesize, 0, videoHeight,
            rgbFrame->data, rgbFrame->linesize);
    return rgbFrame;
};

void frame_to_texture(AVFrame * frame, GLuint texID, GLuint slot) {
    // 在OpenGL中创建纹理
    glActiveTexture(slot);
    glBindTexture(GL_TEXTURE_2D, texID);
    printf("width %d, height %d %d\n", codec_ctx->width, codec_ctx->height, 
    frame->linesize[0]);
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, codec_ctx->width, codec_ctx->height, 0, GL_RGB, GL_UNSIGNED_BYTE, &frame->data[0][0]);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
}



int decoder_thread(AVFormatContext *fmt_ctx, std::vector<AVFrame*> &queue, std::mutex &mtx) {
    static AVPacket *pkt = NULL;
    if (NULL == pkt) {
        pkt = av_packet_alloc();
    }
    int ret = 0;
    if((ret = av_read_frame(fmt_ctx, pkt)) >= 0) {
        int ret = 0;
        static AVFrame *frame;
        if (frame == NULL) {
            frame = av_frame_alloc();
        }
        // 把AVPacket 送进 解码器
        if (vstream_id == pkt->stream_index) {
        } else {
            return 0;
        }

        printf("vsid %d, psid %d\n", vstream_id, pkt->stream_index);
        ret = avcodec_send_packet(codec_ctx, pkt);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE]{0};
            av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
            fprintf(stderr, "Error submitting a packet for decoding (%s)\n", errbuf);
            return -1;
        }

        do {
            ret = avcodec_receive_frame(codec_ctx, frame);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                fprintf(stderr, "no error or no data\n");
                return -2;
            }
            if (ret < 0) {
                fprintf(stderr, "failed to receive frame from dec\n");
                av_packet_unref(pkt);
                return -1;
            }

            const char* pix_fmt_name = av_get_pix_fmt_name((enum AVPixelFormat)frame->format);
            fprintf(stderr, "frame format: %s, %d \n", pix_fmt_name, frame->pts);
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                AVFrame* rgb_frame = vframe_to_rgb(frame);
                std::unique_lock<std::mutex> lk(mtx);
                queue.push_back(rgb_frame);

                tick_clock(ck);
                int64_t pts = transform_clock_to_video_pts(ck, codec_ctx->time_base.den, codec_ctx->time_base.num, 30);
                printf("pts: %llu\n", pts);
                {
                    if (false == video_queue.empty()) {
                        fprintf(stderr, "should update\n");
                        should_update = 1;
                    }
                }
            }
            fprintf(stderr, "dec end1\n");
            av_frame_unref(frame);
        }while (ret >= 0);
        fprintf(stderr, "dec end\n");
        av_packet_unref(pkt);
        // update timer
    }
    return 0;
}

int main() {
    int xpos, ypos, width, height;
    const char *description;
    GLFWwindow *window = NULL;

    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed\n");
        exit(1);
    }

    create_clock(&ck, TIMEUNIT::MILLISECONDS, nullptr);

    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), 
        &xpos, &ypos, &width, &height);
    
    const int h = height / 2;
    const int w = width / 2;

    xpos += 50;
    ypos += 50;

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

float vertices[] = {
    // 顶点位置    // 纹理坐标
    -0.7f,  0.7f, 0.f,   0.0f, 1.0f,  // 左上角
    -0.7f, -0.7f, 0.f,   0.0f, 0.0f,  // 左下角
     0.7f, -0.7f, 0.f,   1.0f, 0.0f,  // 右下角

    -0.7f,  0.7f, 0.f,   0.0f, 1.0f,  // 左上角
     0.7f, -0.7f, 0.f,   1.0f, 0.0f,  // 右下角
     0.7f,  0.7f, 0.f,   1.0f, 1.0f   // 右上角
};

    glm::mat4 proj = glm::mat4(1.f);//glm::ortho(-1.0f, 1.0f, -1.f, 1.f, -10.f, 10.f);
    glm::mat4 model = glm::mat4(1.f);
    glm::mat4 view = glm::mat4(1.f);

    glm::mat4 mvp = model * view * proj;

    GLCALL(glClearColor(1.f, 1.f, 1.f, 1.f));
    GLCALL(glViewport(0, 0, 800, 600));

    Shader shader("./video.glsl");
    VertexArray vao;
    VertexBuffer vbo;
    ElementAttribute attribs;
    attribs.Push<float>(3); // vertex 
    attribs.Push<float>(2); // texture 
    vao.Use();
    vbo.Use();
    vao.SetAttribute(attribs);
    vbo.SetData(vertices, sizeof(float) * 30, GL_FLOAT);

    vao.UnUse();
    vbo.UnUse();

    GLuint texID;
    GLCALL(glGenTextures(1, &texID));

    // 视频初始化

    OpenFile("test.mp4", &fmt_ctx, &codec_ctx);
    
    //std::thread dec_thread(decoder_thread, fmt_ctx, std::ref(video_queue) ,std::ref(mtx));
    should_update = 1;
    static int cnt = 0;
    while(1) {
        GLCALL(glClear(GL_COLOR_BUFFER_BIT));
        glfwPollEvents();
        shader.Use();
        vao.Use();
        decoder_thread(fmt_ctx, video_queue, mtx);
        if (should_update.load()) {
            std::unique_lock<std::mutex> lk(mtx);
            if (!video_queue.empty()) {
                fprintf(stderr, "should up1 not empty\n");
                auto it = video_queue.begin();
                AVFrame *frame = *it;
                video_queue.erase(it);
                lk.unlock();
                frame_to_texture(frame, texID, GL_TEXTURE0);
                av_frame_free(&frame);
                should_update = 0;
                shader.BindTexture("texture1", 0);
                shader.SetMatrix4("mvp", mvp);
            }
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}