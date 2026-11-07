cmake_minimum_required(VERSION 3.24)

project(PanoramaStitching LANGUAGES CXX)

if(NOT MSVC OR NOT CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
    message(FATAL_ERROR "Requires MSVC x64")
endif()

find_package(OpenCV 4.14.0 EXACT REQUIRED CONFIG
    COMPONENTS core imgproc imgcodecs highgui features2d calib3d
    HINTS "D:/OpenCV/opencv/build"
)
add_executable(panorama_stitching main.cpp)


target_compile_features(panorama_stitching PRIVATE cxx_std_17)

target_include_directories(panorama_stitching PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(panorama_stitching PRIVATE
    ${OpenCV_LIBS}
)

add_custom_command(TARGET panorama_stitching POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:panorama_stitching>
        $<TARGET_FILE_DIR:panorama_stitching>
    COMMAND_EXPAND_LISTS
    VERBATIM
)