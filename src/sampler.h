// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <thread>
#include "common.h"

class CorProfiler;

class Sampler
{
private:
    std::thread m_workerThread;
    static ManualEvent s_waitEvent;

    static void DoSampling(Sampler *sampler, ICorProfilerInfo10* pProfInfo, CorProfiler *parent);

protected:
    ICorProfilerInfo10* pCorProfilerInfo;

    WSTRING GetClassName(ClassID classId);
    WSTRING GetModuleName(ModuleID modId);
    WSTRING GetFunctionName(FunctionID funcID, const COR_PRF_FRAME_INFO frameInfo);

    virtual bool BeforeSampleAllThreads() = 0;
    virtual bool AfterSampleAllThreads() = 0;

    virtual bool SampleThread(ThreadID threadID) = 0;

public:
    Sampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~Sampler() = default;

    void Start();
    void Stop();

    virtual void ThreadCreated(uintptr_t threadId) = 0;
    virtual void ThreadDestroyed(uintptr_t threadId) = 0;
};
