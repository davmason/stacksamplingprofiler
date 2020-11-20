#!/bin/bash

function is_mac()
{
  case $(uname -s) in
    Darwin*)    return 0;;
    *)          return 1;;
  esac
}

export CORECLR_ENABLE_PROFILING=1
export CORECLR_PROFILER={cf0d821e-299b-5307-a3d8-b283c03916dd}

if is_mac; then
    echo "Mac!"
    export CORECLR_PROFILER_PATH=~/work/stacksamplingprofiler/libCorProfiler.dylib
else
    echo "Linux!"
    export CORECLR_PROFILER_PATH=/stacksamplingprofiler/libCorProfiler.so
fi


# Portable thread pool is causing issues
export COMPlus_ThreadPool_UsePortableThreadPool=0

export STACKSAMPLER_ASYNC=1

# if [ -f "$SCRIPT_PATH/runtime/corerun.exe" ]; then
#     echo "runtime/corerun.exe does not exist, you have to copy it from your local coreclr build!"
#     exit 1
# fi

# $SCRIPT_PATH/runtime/corerun $SCRIPT_PATH/webapp.dll
dotnet run -c release

