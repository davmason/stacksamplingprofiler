// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include "sampler.h"

class SuspendRuntimeSampler : public Sampler
{
protected:
    virtual bool BeforeSampleAllThreads();
    virtual bool AfterSampleAllThreads();

    virtual bool SampleThread(ThreadID threadID);

public:
    SuspendRuntimeSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~SuspendRuntimeSampler() = default;

    virtual void ThreadCreated(uintptr_t threadId);
    virtual void ThreadDestroyed(uintptr_t threadId);

    HRESULT StackSnapshotCallback(FunctionID funcId,
        UINT_PTR ip,
        COR_PRF_FRAME_INFO frameInfo,
        ULONG32 contextSize,
        BYTE context[],
        void* clientData);
};

