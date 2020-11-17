cmake_minimum_required (VERSION 3.14)

project(CorProfiler)

if(APPLE)
    set(BASE_SOURCES src/sampler_macos.cpp)
    add_compile_options(-Wno-invalid-noreturn -Wno-pragma-pack -Wno-int-to-pointer-cast -fPIC -fms-extensions -DBIT64 -DPAL_STDCPP_COMPAT -DPLATFORM_UNIX -DHOST_64BIT -std=c++17)

    include_directories($ENV{CORECLR_PATH}/src/pal/inc/rt $ENV{CORECLR_PATH}/src/pal/inc $ENV{CORECLR_PATH}/src/inc)
endif(APPLE)

if (LINUX)
    set(BASE_SOURCES src/sampler_linux.cpp)
    add_compile_options(-Wno-invalid-noreturn -Wno-pragma-pack -Wno-int-to-pointer-cast -fPIC -fms-extensions -DBIT64 -DPAL_STDCPP_COMPAT -DPLATFORM_UNIX -DHOST_64BIT -std=c++17)

    include_directories($ENV{CORECLR_PATH}/src/pal/inc/rt $ENV{CORECLR_PATH}/src/pal/inc $ENV{CORECLR_PATH}/src/inc)
endif(LINUX)

if (WIN32)
    set(BASE_SOURCES src/CorProfiler.def)
endif(WIN32)

include_directories($CORECLR_BIN/inc $ENV{CORECLR_PATH}/src/pal/prebuilt/inc)

set(SOURCES ${BASE_SOURCES} src/common.cpp src/ClassFactory.cpp src/CorProfiler.cpp src/dllmain.cpp src/sampler.cpp src/suspendruntime_sampler.cpp src/async_sampler.cpp $ENV{CORECLR_PATH}/src/pal/prebuilt/idl/corprof_i.cpp)

add_library(CorProfiler SHARED ${SOURCES})