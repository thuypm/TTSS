cmake_minimum_required(VERSION 3.20)

project(pmt_scanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(OpenMP)

add_executable(pmt-scanner
    src/main.cpp
    src/Annotator.cpp
    src/Config.cpp
    src/CropAndWarp.cpp
    src/ImageFileScanner.cpp
    src/ImageProcessor.cpp
    src/ResultWriter.cpp
    src/RoiDetector.cpp
)

target_include_directories(pmt-scanner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(pmt-scanner PRIVATE ${OpenCV_LIBS})

if(OpenMP_CXX_FOUND)
    target_link_libraries(pmt-scanner PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(pmt-scanner PRIVATE PMT_SCANNER_HAS_OPENMP=1)
endif()
