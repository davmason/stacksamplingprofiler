@echo off
setlocal

echo Deleting bin\
if exist bin\ (
    rmdir /s /q bin\
)

echo Deleting CorProfiler.dll
if exist CorProfiler.dll (
    del CorProfiler.dll
)

echo Done!