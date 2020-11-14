// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once


#include <pthread.h>
#include <array>
#include "sampler.h"

class AsyncSampler : public Sampler
{
private:
    static AutoEvent s_threadSampledEvent;
    static AsyncSampler *s_instance;

    ThreadSafeMap<uintptr_t, pthread_t> threadIDMap;
    // std::array<uint8_t, 32 * 1024> stackBuffer;

    static void SignalHandler(int signal, siginfo_t *info, void *unused);

protected:
    virtual bool BeforeSampleAllThreads();
    virtual bool AfterSampleAllThreads();

    virtual bool SampleThread(ThreadID threadID);

public:
    AsyncSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~AsyncSampler() = default;

    virtual void ThreadCreated(uintptr_t threadId);
    virtual void ThreadDestroyed(uintptr_t threadId);
};
