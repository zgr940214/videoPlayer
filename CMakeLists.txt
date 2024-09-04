cmake_minimum_required(VERSION 3.25)

project(videoPlayer)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")

file(GLOB_RECURSE src 
    ${CMAKE_SOURCE_DIR}/src/*.c*
    ${CMAKE_SOURCE_DIR}/src/*.h*
)

add_executable(${PROJECT_NAME}
${CMAKE_SOURCE_DIR}/src/test.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/src/audio/
    ${CMAKE_SOURCE_DIR}/src/gl/
    ${CMAKE_SOURCE_DIR}/src/video/
    ${CMAKE_SOURCE_DIR}/src/base/
    ${CMAKE_SOURCE_DIR}/dependency/ffmpeg/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/dependency/
    ${CMAKE_SOURCE_DIR}/dependency/ffmpeg/lib/
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
    
    swresample 
    swscale
    avcodec
    avformat
    avutil
)