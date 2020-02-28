Cross platform stack sampling CoreCLR Profiler
===============================

This sample shows a CoreCLR profiler that suspends the runtime with `ICorProfilerInfo10::SuspendRuntime`, then walks all managed stacks with `ICorProfilerInfo2::DoStackSnapshot`.
