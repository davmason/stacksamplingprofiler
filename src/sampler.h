// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <thread>
#include <cstdio>
#include "common.h"

class CorProfiler;

class Sampler
{
private:
    static constexpr char const *OutputName = "samples.txt";
    std::thread m_workerThread;
    static ManualEvent s_waitEvent;

    static void DoSampling(Sampler *sampler, ICorProfilerInfo10* pProfInfo, CorProfiler *parent, FILE *outputFile);

protected:
    ICorProfilerInfo10* m_pCorProfilerInfo;
    CorProfiler *m_parent;

    FILE *m_outputFile;

    WSTRING GetClassName(ClassID classId);
    WSTRING GetModuleName(ModuleID modId);
    WSTRING GetFunctionName(FunctionID funcID, const COR_PRF_FRAME_INFO frameInfo);

    virtual bool BeforeSampleAllThreads() = 0;
    virtual bool AfterSampleAllThreads() = 0;

    virtual bool SampleThread(ThreadID threadID) = 0;

public:
    Sampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~Sampler();

    void Start();
    void Stop();

    virtual void ThreadCreated(uintptr_t threadId) = 0;
    virtual void ThreadDestroyed(uintptr_t threadId) = 0;
};
