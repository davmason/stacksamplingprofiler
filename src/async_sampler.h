// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once


#include "sampler.h"

class AsyncSampler : public Sampler
{
protected:
    virtual bool BeforeSampleAllThreads();
    virtual bool AfterSampleAllThreads();

    virtual bool SampleThread(ThreadID threadID);

public:
    AsyncSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~AsyncSampler() = default;
};
