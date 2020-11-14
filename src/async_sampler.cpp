// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "CorProfiler.h"
#include "async_sampler.h"


bool AsyncSampler::BeforeSampleAllThreads()
{
    printf("Async sampler not implemented!");
    return false;
}

bool AsyncSampler::AfterSampleAllThreads()
{
    printf("Async sampler not implemented!");
    return false;
}

bool AsyncSampler::SampleThread(ThreadID threadID)
{
    printf("Async sampler not implemented!");
    return false;
}

AsyncSampler::AsyncSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent) :
    Sampler(pProfInfo, parent)
{

}