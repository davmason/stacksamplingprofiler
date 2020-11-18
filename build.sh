#!/bin/bash

function is_mac()
{
  case $(uname -s) in
    Darwin*)    return 0;;
    *)          return 1;;
  esac
}

if is_mac; then
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    export BuildOS=OSX
    profilerExtension="dylib"

    export RUNTIME_PATH=~/work/runtime
else
    # I'm running linux build on docker so the paths change
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    export BuildOS=Linux
    profilerExtension="so"

    export RUNTIME_PATH=/runtime
fi

export BuildArch=x64
export BuildType=Debug

export CORECLR_PATH=$RUNTIME_PATH/src/coreclr
export CORECLR_BIN=$RUNTIME_PATH/artifacts/bin/coreclr/$BuildOS.$BuildArch.$BuildType

printf '  CORECLR_PATH : %s\n' "$CORECLR_PATH"
printf '  CORECLR_BIN  : %s\n' "$CORECLR_BIN"
printf '  BuildOS      : %s\n' "$BuildOS"
printf '  BuildArch    : %s\n' "$BuildArch"
printf '  BuildType    : %s\n' "$BuildType"

printf '  Building ...\n'

if [ ! -d "bin/" ]; then
    mkdir bin/
fi

pushd bin

cmake ../ -DCMAKE_BUILD_TYPE=Debug

make -j8

popd

printf "  Copying libCorProfiler.$profilerExtension to main directory\n"
cp bin/libCorProfiler.$profilerExtension .

printf 'Done.\n'
