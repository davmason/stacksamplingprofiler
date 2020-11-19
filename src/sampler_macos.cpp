// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <unistd.h>
#include <libproc.h>
#include <cinttypes>

#include "CorProfiler.h"
#include "sampler.h"

ThreadState Sampler::GetThreadState(ThreadID threadID)
{
    pid_t pid = getpid();

    // TODO: why do I have to add the 0xE0?
    uint64_t nativeThreadID = (uint64_t)GetNativeThreadID(threadID) | 0xE0;
    struct proc_threadinfo info;
    int result = proc_pidinfo(pid, PROC_PIDTHREADINFO, nativeThreadID, &info, sizeof(struct proc_threadinfo));
    if (result != sizeof(struct proc_threadinfo))
    {
        fprintf(m_outputFile, "proc_pidinfo returned error for PROC_PIDTHREADINFO result=%d\n", result);
        return ThreadState::Running;
    }

    switch (info.pth_run_state)
    {
        case TH_STATE_RUNNING:
        case TH_STATE_UNINTERRUPTIBLE:
            return ThreadState::Running;
        case TH_STATE_STOPPED:
            return ThreadState::Dead;
        case TH_STATE_WAITING:
        case TH_STATE_HALTED:
            return ThreadState::Suspended;
        default:
            fprintf(m_outputFile, "Unknown thread state %u\n", info.pth_run_state);
            return ThreadState::Running;
    }
}

NativeThreadID Sampler::GetCurrentNativeThreadID()
{
    return GetCurrentPThreadID();
}

void *Sampler::GetCurrentThreadStackBase()
{
    void *stackAddr = pthread_get_stackaddr_np(pthread_self());
    return stackAddr;
}