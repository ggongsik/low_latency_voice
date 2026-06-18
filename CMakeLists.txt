cmake_minimum_required(VERSION 3.22)

project(
  LowLatencyVoiceChanger
  VERSION 0.1.0
  DESCRIPTION "Ultra-low-latency real-time character voice changer engine"
  LANGUAGES CXX)

option(LLVC_BUILD_TESTS "Build unit tests" ON)
option(LLVC_BUILD_BENCHMARKS "Build benchmark command-line tools" ON)
option(LLVC_BUILD_JUCE_APP "Build the JUCE realtime standalone app" OFF)
option(LLVC_FETCH_JUCE "Fetch JUCE with CMake FetchContent when JUCE is not local" OFF)
option(LLVC_ENABLE_ONNXRUNTIME "Enable the ONNX Runtime inference backend" OFF)

set(LLVC_JUCE_DIR
    ""
    CACHE PATH "Path to a local JUCE checkout, if LLVC_BUILD_JUCE_APP is enabled")
set(LLVC_ONNXRUNTIME_ROOT
    ""
    CACHE PATH "Path to an ONNX Runtime installation root")
set(LLVC_ONNXRUNTIME_INCLUDE_DIR
    ""
    CACHE PATH "Path to ONNX Runtime headers")
set(LLVC_ONNXRUNTIME_LIBRARY
    ""
    CACHE FILEPATH "Path to the ONNX Runtime shared/import library")
set(LLVC_ONNXRUNTIME_DLL
    ""
    CACHE FILEPATH "Path to onnxruntime.dll on Windows")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(llvc_core
  src/audio/AudioEngine.cpp
  src/audio/AudioWorkerPipeline.cpp
  src/dsp/PitchYIN.cpp
  src/inference/AudioTensorAdapter.cpp
  src/inference/DummyVoiceConversionBackend.cpp
  src/inference/IVoiceConversionBackend.cpp
  src/inference/OnnxBackend.cpp
  src/profiler/LatencyProfiler.cpp)

target_include_directories(llvc_core PUBLIC src)
target_link_libraries(llvc_core PUBLIC Threads::Threads)

if(LLVC_ENABLE_ONNXRUNTIME)
  if(NOT LLVC_ONNXRUNTIME_INCLUDE_DIR)
    find_path(LLVC_ONNXRUNTIME_INCLUDE_DIR
      NAMES onnxruntime_cxx_api.h
      PATHS
        "${LLVC_ONNXRUNTIME_ROOT}/include"
        "${LLVC_ONNXRUNTIME_ROOT}/include/onnxruntime"
      NO_DEFAULT_PATH)
  endif()

  if(NOT LLVC_ONNXRUNTIME_LIBRARY)
    find_library(LLVC_ONNXRUNTIME_LIBRARY
      NAMES onnxruntime
      PATHS
        "${LLVC_ONNXRUNTIME_ROOT}/lib"
        "${LLVC_ONNXRUNTIME_ROOT}/lib64"
      NO_DEFAULT_PATH)
  endif()

  if(WIN32 AND NOT LLVC_ONNXRUNTIME_DLL)
    find_file(LLVC_ONNXRUNTIME_DLL
      NAMES onnxruntime.dll
      PATHS
        "${LLVC_ONNXRUNTIME_ROOT}/bin"
        "${LLVC_ONNXRUNTIME_ROOT}/lib"
      NO_DEFAULT_PATH)
  endif()

  if(NOT LLVC_ONNXRUNTIME_INCLUDE_DIR OR NOT LLVC_ONNXRUNTIME_LIBRARY)
    message(FATAL_ERROR
      "LLVC_ENABLE_ONNXRUNTIME is ON, but ONNX Runtime was not found. "
      "Set LLVC_ONNXRUNTIME_ROOT or LLVC_ONNXRUNTIME_INCLUDE_DIR and "
      "LLVC_ONNXRUNTIME_LIBRARY.")
  endif()

  target_compile_definitions(llvc_core PUBLIC LLVC_ENABLE_ONNXRUNTIME=1)
  target_include_directories(llvc_core PRIVATE "${LLVC_ONNXRUNTIME_INCLUDE_DIR}")
  target_link_libraries(llvc_core PRIVATE "${LLVC_ONNXRUNTIME_LIBRARY}")

  if(WIN32 AND LLVC_ONNXRUNTIME_DLL)
    message(STATUS "ONNX Runtime DLL: ${LLVC_ONNXRUNTIME_DLL}")
  elseif(WIN32)
    message(WARNING
      "onnxruntime.dll was not found under LLVC_ONNXRUNTIME_ROOT. "
      "Put it on PATH or set LLVC_ONNXRUNTIME_DLL before running executables.")
  endif()
else()
  target_compile_definitions(llvc_core PUBLIC LLVC_ENABLE_ONNXRUNTIME=0)
endif()

function(llvc_copy_onnxruntime_dll target_name)
  if(WIN32 AND LLVC_ENABLE_ONNXRUNTIME AND LLVC_ONNXRUNTIME_DLL)
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LLVC_ONNXRUNTIME_DLL}"
        "$<TARGET_FILE_DIR:${target_name}>"
      COMMENT "Copying ONNX Runtime DLL next to ${target_name}")
  endif()
endfunction()

if(MSVC)
  target_compile_options(llvc_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(llvc_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(LLVC_BUILD_TESTS)
  enable_testing()

  add_executable(llvc_tests
    tests/test_main.cpp
    tests/test_audio_engine.cpp
    tests/test_audio_tensor_adapter.cpp
    tests/test_audio_worker_pipeline.cpp
    tests/test_inference_backends.cpp
    tests/test_latency_profiler.cpp
    tests/test_pitch_yin.cpp
    tests/test_spsc_ring_buffer.cpp)

  target_link_libraries(llvc_tests PRIVATE llvc_core)
  llvc_copy_onnxruntime_dll(llvc_tests)
  add_test(NAME llvc_tests COMMAND llvc_tests)
endif()

if(LLVC_BUILD_BENCHMARKS)
  add_executable(llvc_benchmark_cli
    apps/benchmark_cli/main.cpp
    benchmarks/bench_audio_pipeline.cpp)

  target_link_libraries(llvc_benchmark_cli PRIVATE llvc_core)
  llvc_copy_onnxruntime_dll(llvc_benchmark_cli)
endif()

if(LLVC_BUILD_JUCE_APP)
  include(FetchContent)

  if(LLVC_JUCE_DIR)
    add_subdirectory("${LLVC_JUCE_DIR}" juce)
  elseif(LLVC_FETCH_JUCE)
    FetchContent_Declare(
      JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.8)
    FetchContent_MakeAvailable(JUCE)
  endif()

  if(TARGET juce::juce_audio_utils)
    juce_add_gui_app(llvc_realtime_app
      PRODUCT_NAME "Low Latency Voice Changer")

    target_sources(llvc_realtime_app PRIVATE apps/realtime_app/Main.cpp)
    target_link_libraries(llvc_realtime_app
      PRIVATE
        llvc_core
        juce::juce_audio_utils
        juce::juce_gui_extra
      PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
    target_compile_definitions(llvc_realtime_app
      PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    llvc_copy_onnxruntime_dll(llvc_realtime_app)
  else()
    message(FATAL_ERROR
      "LLVC_BUILD_JUCE_APP is ON, but JUCE was not found. Set LLVC_JUCE_DIR or LLVC_FETCH_JUCE=ON.")
  endif()
endif()
