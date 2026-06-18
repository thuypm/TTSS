cmake_minimum_required(VERSION 3.20)

project(pmt_scanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(OpenMP)

set(PMT_SCANNER_CORE_SOURCES
    src/AnnotatedResultImage.cpp
    src/Annotator.cpp
    src/Config.cpp
    src/CropAndWarp.cpp
    src/ImageFileScanner.cpp
    src/ResultWriter.cpp
    src/RoiDetector.cpp
    src/RoiDetector/Common.cpp
    src/RoiDetector/PartOneTwo.cpp
    src/RoiDetector/PartThree.cpp
    src/RoiDetector/StudentIdKey.cpp
)

add_executable(pmt-scanner
    src/main.cpp
    src/ImageProcessor.cpp
    ${PMT_SCANNER_CORE_SOURCES}
)

add_executable(pmt-scanner-sequential
    src/main_sequential.cpp
    src/ImageProcessorSequential.cpp
    ${PMT_SCANNER_CORE_SOURCES}
)

foreach(target pmt-scanner pmt-scanner-sequential)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(${target} PRIVATE ${OpenCV_LIBS})
endforeach()

if(OpenMP_CXX_FOUND)
    target_link_libraries(pmt-scanner PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(pmt-scanner PRIVATE PMT_SCANNER_HAS_OPENMP=1)
endif()
