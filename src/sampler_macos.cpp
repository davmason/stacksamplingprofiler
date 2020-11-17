// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "CorProfiler.h"
#include "sampler.h"
#include <unistd.h>
#include <libproc.h>
#include <cinttypes>

uint64_t *g_threadInfos = nullptr;
size_t g_threadInfosSize = 200;

int proc_pidinfo(int pid, int flavor, uint64_t arg, user_addr_t buffer, uint32_t buffersize, int32_t * retval);

ThreadState Sampler::GetThreadState(ThreadID threadID)
{
    if (g_threadInfos == nullptr)
    {
        g_threadInfos = new uint64_t[g_threadInfosSize];
    }

    pid_t pid = getpid();

    fprintf(m_outputFile, "Printing all threads\n");
    int result = proc_pidinfo(pid, PROC_PIDLISTTHREADS, 0, g_threadInfos, sizeof(uint64_t) * g_threadInfosSize);
    for (int i = 0; i < result / sizeof(uint64_t); ++i)
    {
        fprintf(m_outputFile, "thread index=%d id=%" PRIx64 "\n", i, g_threadInfos[i]);
    }

    // TODO: why do I have to add the 0xE0?
    uint64_t nativeThreadID = (uint64_t)GetNativeThreadID(threadID) | 0xE0;
    struct proc_threadinfo info;
    fprintf(m_outputFile, "Calling proc_pidinfo nativeThreadID=%" PRIx64 "\n", nativeThreadID);
    result = proc_pidinfo(pid, PROC_PIDTHREADINFO, nativeThreadID, &info, sizeof(struct proc_threadinfo));

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
