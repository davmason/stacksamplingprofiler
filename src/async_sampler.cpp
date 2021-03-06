// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <signal.h>
#include <cinttypes>
#include <locale>
#include <codecvt>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include <dlfcn.h>
#include <execinfo.h>
#include <fstream>
#include <iostream>

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
    // What is this signal handler doing? It is copying the entire stack off for later processing.
    // The stack base is precalculated and stored at thread creation time, since the functions
    // for querying the stack base are not signal safe. This signal handler is not reentrancy safe,
    // pains have been taken to synchronize with the sampling thread so it should never be called
    // in a reentrant manner.

    unw_context_t context;
    unw_getcontext(&context);

    unw_cursor_t cursor;
    unw_init_local(&cursor, &context);

    // Get current stack pointer for the top bound of what to copy
    unw_word_t sp;
    unw_get_reg(&cursor, UNW_REG_SP, &sp);

    // This may cause confusion, since as developers we talk about the top of the stack being
    // the stack associated with the leaf functions, but since the stack grows downwards the
    // top is actually the lowest address and the bottom is the highest.
    uintptr_t stackTop = (uintptr_t)sp;
    uintptr_t stackBottom = (uintptr_t)AsyncSampler::Instance()->m_stackBase;

    // Get current RBP as the value to start the stack walking from. If setting up libunwind
    // proves to be too slow it would be fairly trivial to write an assembly stub that captures it.
    unw_word_t rbp;
    unw_get_reg(&cursor, UNW_X86_64_RBP, &rbp);

    AsyncSampler::Instance()->m_firstRBP = (uintptr_t)rbp;
    uintptr_t stackSize = stackBottom - stackTop;

    // Copy the stack off, limiting to the top 32kb if larger than that
    uintptr_t startIndex = 0;
    if (stackSize >= AsyncSampler::Instance()->m_stack.size())
    {
        printf("here! stackSize=%" PRIu64 " array size=%zu\n", stackSize, AsyncSampler::Instance()->m_stack.size());
        // Copy the top 32k since that's all we have room for
        memcpy((void *)AsyncSampler::Instance()->m_stack.data(), (void *)stackTop, AsyncSampler::Instance()->m_stack.size());

        // This is a little bit of a hack, but makes stack relocation later easier
        // TODO: should actually test this to make sure it works as expected.
        uintptr_t stackDiff = stackSize - AsyncSampler::Instance()->m_stack.size();
        AsyncSampler::Instance()->m_stackBase = stackBottom - stackDiff;
    }
    else
    {
        // Only will partially fill the array
        startIndex = AsyncSampler::Instance()->m_stack.size() - stackSize;
        memcpy((void *)(&AsyncSampler::Instance()->m_stack[startIndex]), (void *)stackTop, stackSize);
    }

    AsyncSampler::Instance()->m_startIndex = startIndex;

    // TODO: mutex use is not signal safe
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

uintptr_t AsyncSampler::MapStackAddressToLocalOffset(uintptr_t address)
{
    // All of the RBPs reference the stack, so when we copied it the stack all of the
    // addresses are still pointing at the old stack. This function does a basic fixup
    // by mapping the old stack value to an index in to the m_stack array.

    uintptr_t offsetFromStackBase = m_stackBase - address;
    return m_stack.size() - offsetFromStackBase;
}

uintptr_t AsyncSampler::ReadPtrSlotFromStack(uintptr_t offset)
{
    // This makes so many assumptions. Endian-ness, 64 bit architecture, etc.
    Int64Parser parser;
    parser.eighth   = m_stack[offset + 7];
    parser.seventh  = m_stack[offset + 6];
    parser.sixth    = m_stack[offset + 5];
    parser.fifth    = m_stack[offset + 4];
    parser.fourth   = m_stack[offset + 3];
    parser.third    = m_stack[offset + 2];
    parser.second   = m_stack[offset + 1];
    parser.first    = m_stack[offset];

    return parser.i;
}

bool AsyncSampler::SampleThread(ThreadID threadID)
{
    // m_stackBase needs to be pre-set so the signal handler can access them without
    // going through the locks in the ThreadSafeMap class.
    pthread_t pThreadID = GetPThreadID(threadID);
    m_stackBase = (uintptr_t)GetStackBase(threadID);
    if (m_stackBase == 0)
    {
        fprintf(m_outputFile, "Don't know stack base for thread, skipping...\n");
        return false;
    }

    // Send the signal, currently using SIGUSR2 but before using in production should verify it's safe
    // and nothing else uses it.
    fprintf(m_outputFile, "Sending signal to thread %p state=%d\n", (void *)pThreadID, GetThreadState(threadID));
    fflush(m_outputFile);
    int result = pthread_kill(pThreadID, SIGUSR2);
    fprintf(m_outputFile, "pthread_kill result=%d\n", result);

    // Only sample one thread at a time
    s_threadSampledEvent.Wait();

    fprintf(m_outputFile, "starting manual RBP stack unwind...\n");

    uintptr_t rbp = MapStackAddressToLocalOffset(m_firstRBP);
    while (true)
    {
        // This sampler only works for AMD64 currently, and works by walking the RBP chain.
        // RBP points to a stack slot that has the previous functions RBP in it, RBP+1 (or +8 here
        // since m_stack is in bytes) contains the IP to return to.
        //
        // The algorithm is:
        //      rbp = *rbp
        //      ip = *(rbp + 8)
        //
        // Then we can map the IP to either managed or native code.

        uintptr_t ipOffset = rbp + 8;
        uintptr_t ip = ReadPtrSlotFromStack(ipOffset);
        FunctionID functionID;
        HRESULT hr = m_pCorProfilerInfo->GetFunctionFromIP((uint8_t *)ip, &functionID);
        if (hr == S_OK)
        {
            // We found managed code, look up the function name to print it out.
            WSTRING functionName = s_instance->GetFunctionName(functionID, NULL);

    #if WIN32
            wstring_convert<codecvt_utf8<wchar_t>, wchar_t> convert;
    #else // WIN32
            wstring_convert<codecvt_utf8<char16_t>, char16_t> convert;
    #endif // WIN32

            string printable = convert.to_bytes(functionName);
            fprintf(m_outputFile, "%s (funcId=0x%" PRIx64 ")\n", printable.c_str(), (uint64_t)functionID);

        }
        else
        {
            // If GetFunctionFromIP returned an error, we are assuming it means native code. Here
            // we are borrowing dladdr to look up symbolic names, but if native code doesn't matter
            // to your profiler then feel free to omit this and skip to the next frame to save some
            // perf cost.
            const char *nativeName = "Unknown";
            Dl_info info;
            int result = dladdr((void *)ip, &info);
            if (result != 0)
            {
                nativeName = info.dli_sname;
            }

            uintptr_t offset = ip - (uintptr_t)info.dli_saddr;
            fprintf(m_outputFile, "Native frame \"%s+0x%" PRIx64 "\" ip=%" PRIx64 "\n", nativeName, offset, ip);
         }

        uintptr_t newRbpRaw = ReadPtrSlotFromStack(rbp);
        rbp = MapStackAddressToLocalOffset(newRbpRaw);
        if (rbp < m_startIndex || rbp >= m_stack.size())
        {
            fprintf(m_outputFile, "done, rbp=%" PRIx64 "\n", rbp);
            break;
        }
    }

    return true;
}

AsyncSampler::AsyncSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent) :
    Sampler(pProfInfo, parent),
    m_stack(),
    m_stackBase(0),
    m_firstRBP(0),
    m_startIndex(0)
{
    s_instance = this;

    struct sigaction sampleAction;
    sampleAction.sa_flags = 0;
    sampleAction.sa_sigaction = AsyncSampler::SignalHandler;
    sampleAction.sa_flags |= SA_SIGINFO;
    sigemptyset(&sampleAction.sa_mask);
    sigaddset(&sampleAction.sa_mask, SIGUSR2);

    struct sigaction oldAction;
    int result = sigaction(SIGUSR2, &sampleAction, &oldAction);
}
