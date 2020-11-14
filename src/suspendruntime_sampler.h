// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <thread>
#include "sampler.h"

class CorProfiler;

class SuspendRuntimeSampler : public Sampler
{
private:
    static SuspendRuntimeSampler* s_instance;

    std::thread m_workerThread;
    static ManualEvent s_waitEvent;

    ICorProfilerInfo10* corProfilerInfo;

    static void DoSampling(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);

    WSTRING GetClassName(ClassID classId);
    WSTRING GetModuleName(ModuleID modId);
    WSTRING GetFunctionName(FunctionID funcID, const COR_PRF_FRAME_INFO frameInfo);
public:
    static SuspendRuntimeSampler* Instance()
    {
        return s_instance;
    }

    SuspendRuntimeSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~SuspendRuntimeSampler() = default;

    virtual void Start();
    virtual void Stop();

    HRESULT StackSnapshotCallback(FunctionID funcId,
        UINT_PTR ip,
        COR_PRF_FRAME_INFO frameInfo,
        ULONG32 contextSize,
        BYTE context[],
        void* clientData);
};

