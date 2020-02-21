@echo off
setlocal

echo.
echo Building test app (multithreadedapp.dll)

pushd multithreadedapp
dotnet build -c release
dotnet publish -r win10-x64 -c release
popd

echo.
echo Building profiler library 

call build.cmd

if exist repro\ (
    rmdir /s /q repro
)

mkdir repro

pushd repro

mkdir runtime

echo.
echo Copying repro.cmd to repro folder
copy ..\src\windows\repro.cmd .

echo.
echo Copying CorProfier.dll to repro folder
copy ..\CorProfiler.dll .

echo.
echo Copying published files to runtime folder
xcopy /y /e /q ..\multithreadedapp\bin\release\netcoreapp3.1\win10-x64\* runtime\

echo.
echo Copying app files to repro folder
xcopy /y /q ..\multithreadedapp\bin\release\netcoreapp3.1\* .

if ["%~1"]==[""] (
    echo Did not pass a path to a coreclr repo, skipping copying private bits
    goto :EOF
)

echo Copying coreclr from %1*
xcopy /y /e /q %1* runtime\

popd