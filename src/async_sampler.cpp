// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <signal.h>
#include <cinttypes>
#include <locale>
#include <codecvt>
#include <libunwind.h>
#include <execinfo.h>
#include <fstream>
#include <iostream>
#include <boost/stacktrace.hpp>

#include "CorProfiler.h"
#include "async_sampler.h"

using std::wstring_convert;
using std::codecvt_utf8;
using std::string;
using std::ifstream;

AutoEvent AsyncSampler::s_threadSampledEvent;
AsyncSampler *AsyncSampler::s_instance;

//static
void AsyncSampler::SignalHandler(int signal, siginfo_t *info, void *unused)
{
    NativeThreadID nativeThreadID = AsyncSampler::Instance()->GetCurrentNativeThreadID();
    AsyncSampler::Instance()->m_stackThreadID = nativeThreadID;

    // // Save backtrace for later processing
    // AsyncSampler::Instance()->m_numStackIPs = backtrace(AsyncSampler::Instance()->m_stackIPs.data(), AsyncSampler::Instance()->m_stackIPs.size());

    printf("Boost stacktrace:\n");
    std::cout << boost::stacktrace::stacktrace() << std::endl;
    // int i = 0;
    // for (boost::stacktrace::frame frame: st)
    // {
    //     AsyncSampler::Instance()->m_stackIPs[i] = (void *)frame.address();
    //     ++i;
    // }

    // AsyncSampler::Instance()->m_numStackIPs = i;

    s_threadSampledEvent.Signal();
}

bool AsyncSampler::BeforeSampleAllThreads()
{
    return true;
}

bool AsyncSampler::AfterSampleAllThreads()
{
    return true;
}

bool AsyncSampler::SampleThread(ThreadID threadID)
{
    pthread_t pThreadID = GetPThreadID(threadID);

    fprintf(m_outputFile, "Sending signal to thread %p state=%d\n", (void *)pThreadID, GetThreadState(threadID));
    fflush(m_outputFile);
    int result = pthread_kill(pThreadID, SIGUSR2);
    fprintf(m_outputFile, "pthread_kill result=%d\n", result);

    // Only sample one thread at a time
    s_threadSampledEvent.Wait();

    char **nativeSymbols = backtrace_symbols(m_stackIPs.data(), m_numStackIPs);
    for (int i = 0; i < m_numStackIPs; ++i)
    {
        // Managed name
        FunctionID functionID;
        HRESULT hr = m_pCorProfilerInfo->GetFunctionFromIP((uint8_t *)m_stackIPs[i], &functionID);
        if (hr != S_OK)
        {
            // If it's not managed, must be native
            fprintf(m_outputFile, "    %s\n", nativeSymbols[i]);
            continue;
        }

        WSTRING functionName = s_instance->GetFunctionName(functionID, NULL);

#if WIN32
        wstring_convert<codecvt_utf8<wchar_t>, wchar_t> convert;
#else // WIN32
        wstring_convert<codecvt_utf8<char16_t>, char16_t> convert;
#endif // WIN32

        string printable = convert.to_bytes(functionName);
        fprintf(m_outputFile, "    %d  %s (funcId=0x%" PRIx64 ")\n", i, printable.c_str(), (uint64_t)functionID);
    }

    free(nativeSymbols);

    return true;
}

AsyncSampler::AsyncSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent) :
    Sampler(pProfInfo, parent),
    m_stackIPs(),
    m_numStackIPs(0),
    m_stackThreadID(0)
{
    s_instance = this;

    struct sigaction sampleAction;
    sampleAction.sa_flags = 0;
    // sampleAction.sa_handler = AsyncSampler::SignalHandler;
    sampleAction.sa_sigaction = AsyncSampler::SignalHandler;
    sampleAction.sa_flags |= SA_SIGINFO;
    sigemptyset(&sampleAction.sa_mask);
    sigaddset(&sampleAction.sa_mask, SIGUSR2);

    struct sigaction oldAction;
    int result = sigaction(SIGUSR2, &sampleAction, &oldAction);
    // fprintf(m_outputFile, "sigaction result=%d\n", result);
}
