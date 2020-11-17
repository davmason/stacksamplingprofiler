#!/bin/bash

export CORECLR_ENABLE_PROFILING=1
export CORECLR_PROFILER={cf0d821e-299b-5307-a3d8-b283c03916dd}

export SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export CORECLR_PROFILER_PATH=$SCRIPT_PATH/libCorProfiler.dylib

# Portable thread pool is causing issues
export COMPlus_ThreadPool_UsePortableThreadPool=0

export STACKSAMPLER_ASYNC=1

# if [ -f "$SCRIPT_PATH/runtime/corerun.exe" ]; then
#     echo "runtime/corerun.exe does not exist, you have to copy it from your local coreclr build!"
#     exit 1
# fi

# $SCRIPT_PATH/runtime/corerun $SCRIPT_PATH/webapp.dll
dotnet run -c release -p webapp/webapp.csproj