// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once


#include <pthread.h>
#include <array>
#include <signal.h>

#include "sampler.h"

class AsyncSampler : public Sampler
{
private:
    typedef union
    {
        struct
        {
            unsigned int first    : 8;
            unsigned int second   : 8;
            unsigned int third    : 8;
            unsigned int fourth   : 8;
            unsigned int fifth    : 8;
            unsigned int sixth    : 8;
            unsigned int seventh  : 8;
            unsigned int eighth   : 8;
        };

        uint64_t i;
    } Int64Parser;

    static AutoEvent s_threadSampledEvent;
    static AsyncSampler *s_instance;

    std::array<uint8_t, 32768> m_stack;
    uintptr_t m_stackBase;
    uintptr_t m_firstRBP;
    uintptr_t m_startIndex;
    NativeThreadID m_stackThreadID;

    static void SignalHandler(int signal, siginfo_t *info, void *unused);

    uintptr_t MapStackAddressToLocalOffset(uintptr_t address);
    uintptr_t ReadPtrSlotFromStack(uintptr_t offset);

protected:
    virtual bool BeforeSampleAllThreads();
    virtual bool AfterSampleAllThreads();

    virtual bool SampleThread(ThreadID threadID);

public:
    static AsyncSampler *Instance()
    {
        return s_instance;
    }

    AsyncSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~AsyncSampler() = default;
};
