// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "CorProfiler.h"
#include "sampler.h"
#include <fstream>
#include <iostream>

using std::ifstream;

ThreadState Sampler::GetThreadState(ThreadID threadID)
{
    fprintf(m_outputFile, "Contents of /proc/self/task/[tid]/stat for thread %p\n", nativeThreadId);
    string fileName = "/proc/self/task/" + std::to_string((uintptr_t)nativeThreadId) + "/stat";
    ifstream threadState(fileName);
    string line;
    while (getline(threadState, line))
    {
        fprintf(m_outputFile, "%s\n", line.c_str());
    }

}
