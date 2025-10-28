cmake_minimum_required(VERSION 3.22.1)

project(beatriceAndroid LANGUAGES C CXX)

find_package (oboe REQUIRED CONFIG)

get_filename_component(PROJECT_ROOT_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. ABSOLUTE)

set( BEATRICE_VST_DIR ${PROJECT_ROOT_DIR}/lib/beatrice-vst )
set( OBOE_DIR ${PROJECT_ROOT_DIR}/lib/oboe )
set( ASIO_DIR ${PROJECT_ROOT_DIR}/lib/asio )

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        beatriceEngine.cpp
        beatriceFullDuplexPass.cpp
        
        ${OBOE_DIR}/samples/debug-utils/trace.cpp

        ${BEATRICE_VST_DIR}/src/common/processor_core_0.cc
        ${BEATRICE_VST_DIR}/src/common/processor_core_1.cc
        ${BEATRICE_VST_DIR}/src/common/processor_core_2.cc
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${OBOE_DIR}/samples/debug-utils/
        ${OBOE_DIR}/include
        ${BEATRICE_VST_DIR}/lib
        ${BEATRICE_VST_DIR}/src
        ${ASIO_DIR}/asio/include/
)

find_library( log-lib log )
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        oboe::oboe
        android
        atomic
        log
        beatrice
        ${log-lib}
)
target_link_directories( ${CMAKE_PROJECT_NAME} PUBLIC . )

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,-z,max-page-size=16384")
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall "$<$<CONFIG:RELEASE>:-Ofast>")

