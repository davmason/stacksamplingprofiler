#pragma once


#include "sampler.h"

class AsyncSampler : public Sampler
{
private:
    ICorProfilerInfo10* corProfilerInfo;
    CorProfiler *parent;
#error TODO, store these?

public:
    AsyncSampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent);
    virtual ~AsyncSampler() = default;

    virtual void Start()
    {

    }

    virtual void Stop()
    {

    }
};
