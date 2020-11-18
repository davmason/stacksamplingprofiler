#!/bin/bash

docker run -it -v ~/work/stacksamplingprofiler/:/stacksamplingprofiler -v ~/work/runtime_linux:/runtime -v /Users/dave/work/FrameworkBenchmarks/frameworks/CSharp/aspnetcore/Benchmarks:/benchmarks -w /stacksamplingprofiler davmason-ubuntu bash
