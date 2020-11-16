// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "CorProfiler.h"
#include "suspendruntime_sampler.h"
#include <thread>
#include <cwchar>
#include <cstdio>
#include <cinttypes>
#include <locale>
#include <codecvt>

using std::wstring_convert;
using std::codecvt_utf8;
using std::string;

static HRESULT __stdcall DoStackSnapshotStackSnapShotCallbackWrapper(
    FunctionID funcId,
    UINT_PTR ip,
    COR_PRF_FRAME_INFO frameInfo,
    ULONG32 contextSize,
    BYTE context[],
    void* clientData)
{
    assert(clientData != nullptr);

    SuspendRuntimeSampler *myPtr = reinterpret_cast<SuspendRuntimeSampler *>(clientData);
    return myPtr->StackSnapshotCallback(funcId,
        ip,
        frameInfo,
        contextSize,
        context,
        clientData);
}

bool SuspendRuntimeSampler::BeforeSampleAllThreads()
{
    printf("Suspending runtime\n");
    HRESULT hr = pCorProfilerInfo->SuspendRuntime();
    if (FAILED(hr))
    {
        printf("Error suspending runtime... hr=0x%x \n", hr);
        return false;
    }

    return true;
}

bool SuspendRuntimeSampler::AfterSampleAllThreads()
{
    printf("Resuming runtime\n");
    HRESULT hr = pCorProfilerInfo->ResumeRuntime();
    if (FAILED(hr))
    {
        printf("ResumeRuntime failed with hr=0x%x \n", hr);
        return false;
    }

    return true;
}

bool SuspendRuntimeSampler::SampleThread(ThreadID threadID)
{
   HRESULT hr = pCorProfilerInfo->DoStackSnapshot(threadID,
                                                  DoStackSnapshotStackSnapShotCallbackWrapper,
                                                  COR_PRF_SNAPSHOT_REGISTER_CONTEXT,
                                                  (void *)this,
                                                  NULL,
                                                  0);
    if (FAILED(hr))
    {
        if (hr == E_FAIL)
        {
            printf("Managed thread id=0x%" PRIx64 " has no managed frames to walk \n", (uint64_t)threadID);
        }
        else
        {
            printf("DoStackSnapshot for thread id=0x%" PRIx64 " failed with hr=0x%x \n", (uint64_t)threadID, hr);
        }
    }

    return true;
}


SuspendRuntimeSampler::SuspendRuntimeSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent) :
    Sampler(pProfInfo, parent)
{

}

void SuspendRuntimeSampler::ThreadCreated(uintptr_t threadId)
{
    // Nothing to do
}

void SuspendRuntimeSampler::ThreadDestroyed(uintptr_t threadId)
{
    // Nothing to do
}

HRESULT SuspendRuntimeSampler::StackSnapshotCallback(FunctionID funcId, UINT_PTR ip, COR_PRF_FRAME_INFO frameInfo, ULONG32 contextSize, BYTE context[], void* clientData)
{
    WSTRING functionName = GetFunctionName(funcId, frameInfo);

#if WIN32
    wstring_convert<codecvt_utf8<wchar_t>, wchar_t> convert;
#else // WIN32
    wstring_convert<codecvt_utf8<char16_t>, char16_t> convert;
#endif // WIN32

    string printable = convert.to_bytes(functionName);
    printf("    %s (funcId=0x%" PRIx64 ")\n", printable.c_str(), (uint64_t)funcId);
    return S_OK;
}
