#!/bin/bash

export CORECLR_ENABLE_PROFILING=1
export CORECLR_PROFILER={cf0d821e-299b-5307-a3d8-b283c03916dd}
if [ -z "$1" ]; then
    echo "Provide the path to the profiler as an argument"
    exit 1
fi

export CORECLR_PROFILER_PATH=$1

if [ -f "runtime/corerun.exe" ]; then
    echo "runtime/corerun.exe does not exist, you have to copy it from your local coreclr build!"
    exit 1
fi

runtime/corerun multithreadedapp.dll