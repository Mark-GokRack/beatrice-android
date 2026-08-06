cmake_minimum_required(VERSION 3.22.1)

project(beatriceapp LANGUAGES C CXX)

find_package (oboe REQUIRED CONFIG)

get_filename_component(PROJECT_ROOT_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../.. ABSOLUTE)

set( BEATRICE_API_DIR ${PROJECT_ROOT_DIR}/lib/beatrice-api/${ANDROID_ABI} )
set( BEATRICE_VST_DIR ${PROJECT_ROOT_DIR}/lib/beatrice-vst )
set( OBOE_DIR ${PROJECT_ROOT_DIR}/lib/oboe )
set( ASIO_DIR ${PROJECT_ROOT_DIR}/lib/asio )
set( RNNOISE_DIR ${PROJECT_ROOT_DIR}/lib/rnnoise )

add_library(rnnoise STATIC
        ${RNNOISE_DIR}/src/denoise.c
        ${RNNOISE_DIR}/src/rnn.c
        ${RNNOISE_DIR}/src/pitch.c
        ${RNNOISE_DIR}/src/kiss_fft.c
        ${RNNOISE_DIR}/src/celt_lpc.c
        ${RNNOISE_DIR}/src/nnet.c
        ${RNNOISE_DIR}/src/nnet_default.c
        ${RNNOISE_DIR}/src/parse_lpcnet_weights.c
        ${RNNOISE_DIR}/src/rnnoise_data.c
        ${RNNOISE_DIR}/src/rnnoise_tables.c
)

target_include_directories(rnnoise
    PUBLIC
        ${RNNOISE_DIR}/include
    PRIVATE
        ${RNNOISE_DIR}/src
)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        beatriceProcessor.cpp
        beatriceAudioEngine.cpp

        effectors/Amplifier.cpp
        effectors/DynamicProcessor.cpp
        effectors/Compressor.cpp
        effectors/Limiter.cpp
        effectors/NoiseGate.cpp
        effectors/ParametricEqualizer.cpp
        effectors/AudioEffectorChain.cpp
        effectors/RNNoiseProcessor.cpp

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
        ${RNNOISE_DIR}/include
)

find_library( log-lib log )
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        oboe::oboe
        rnnoise
        android
        atomic
        log
        beatrice
        c++_shared
        ${log-lib}
)
target_link_directories( ${CMAKE_PROJECT_NAME} PUBLIC ${BEATRICE_API_DIR} )

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,-z,max-page-size=16384")
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall "$<$<CONFIG:RELEASE>:-Ofast>")

