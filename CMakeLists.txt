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

set(LLVC_JUCE_DIR
    ""
    CACHE PATH "Path to a local JUCE checkout, if LLVC_BUILD_JUCE_APP is enabled")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(llvc_core
  src/audio/AudioEngine.cpp
  src/dsp/PitchYIN.cpp
  src/profiler/LatencyProfiler.cpp)

target_include_directories(llvc_core PUBLIC src)

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
    tests/test_latency_profiler.cpp
    tests/test_spsc_ring_buffer.cpp)

  target_link_libraries(llvc_tests PRIVATE llvc_core)
  add_test(NAME llvc_tests COMMAND llvc_tests)
endif()

if(LLVC_BUILD_BENCHMARKS)
  add_executable(llvc_benchmark_cli
    apps/benchmark_cli/main.cpp
    benchmarks/bench_audio_pipeline.cpp)

  target_link_libraries(llvc_benchmark_cli PRIVATE llvc_core)
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
  else()
    message(FATAL_ERROR
      "LLVC_BUILD_JUCE_APP is ON, but JUCE was not found. Set LLVC_JUCE_DIR or LLVC_FETCH_JUCE=ON.")
  endif()
endif()
