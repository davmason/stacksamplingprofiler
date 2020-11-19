// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <thread>
#include <cstdio>
#include <atomic>
#include <vector>
#include <utility>
#include <pthread.h>

#include "common.h"

class CorProfiler;

enum class ThreadState
{
    Running = 1,
    Suspended = 2,
    Dead = 3
};



#ifdef __APPLE__
    typedef uint32_t NativeThreadID;
#elif __linux__
    typedef pid_t NativeThreadID;
#endif

typedef struct
{
    pthread_t pThreadID;
    NativeThreadID threadID;
    void *stackBase;
} NativeThreadInfo;

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
    ThreadSafeMap<uintptr_t, NativeThreadInfo> m_threadIDMap;

    WSTRING GetClassName(ClassID classId);
    WSTRING GetModuleName(ModuleID modId);
    WSTRING GetFunctionName(FunctionID funcID, const COR_PRF_FRAME_INFO frameInfo);

    ThreadState GetThreadState(ThreadID threadID);

    pthread_t GetCurrentPThreadID();
    pthread_t GetPThreadID(ThreadID threadID);
    NativeThreadID GetCurrentNativeThreadID();
    NativeThreadID GetNativeThreadID(ThreadID threadID);
    void * GetStackBase(ThreadID threadID);

    virtual bool BeforeSampleAllThreads() = 0;
    virtual bool AfterSampleAllThreads() = 0;

    virtual bool SampleThread(ThreadID threadID) = 0;

public:
    Sampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~Sampler();

    void Start();
    void Stop();

    virtual void ThreadCreated(ThreadID threadId);
    virtual void ThreadDestroyed(ThreadID threadId);
};
