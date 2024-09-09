cmake_minimum_required(VERSION 3.25)

project(videoPlayer)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")

file(GLOB_RECURSE src 
    ${CMAKE_SOURCE_DIR}/src/*.c*
    ${CMAKE_SOURCE_DIR}/src/*.h*
)

file(GLOB gl 
    ${CMAKE_SOURCE_DIR}/src/gl/*.c*
    ${CMAKE_SOURCE_DIR}/src/clock.cc
)

add_executable(${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/src/video/test.cpp
    ${CMAKE_SOURCE_DIR}/glad/glad.c
    ${gl}
    #${CMAKE_SOURCE_DIR}/demuxing_example.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src/audio/
    ${CMAKE_SOURCE_DIR}/src/gl/
    ${CMAKE_SOURCE_DIR}/glad
    ${CMAKE_SOURCE_DIR}/glm
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/video/
    ${CMAKE_SOURCE_DIR}/src/base/
    ${CMAKE_SOURCE_DIR}/dependecy/include
    ${CMAKE_SOURCE_DIR}/dependency/ffmpeg/include
    ${CMAKE_SOURCE_DIR}/dependency/GLFW/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/dependency/
    ${CMAKE_SOURCE_DIR}/dependency/GLFW/lib/
    ${CMAKE_SOURCE_DIR}/dependency/ffmpeg/lib/
    ${CMAKE_SOURCE_DIR}/dependency/lib
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    mfplat
    mf
    mfuuid
    wmcodecdspuuid
    ws2_32
    Secur32
    Bcrypt
    strmiids
    ole32
    
    avcodec
    avformat
    avutil
    swresample 
    swscale
    opengl32
    glfw3
    fdk-aac
    libx264
)