// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "CorProfiler.h"
#include "sampler.h"

#include <cstdio>
#include <cinttypes>

ManualEvent Sampler::s_waitEvent;

WSTRING Sampler::GetModuleName(ModuleID modId)
{
    WCHAR moduleFullName[STRING_LENGTH];
    ULONG nameLength = 0;
    AssemblyID assemID;

    if (modId == NULL)
    {
        printf("NULL modId passed to GetModuleName\n");
        return WSTR("Unknown");
    }

    HRESULT hr = pCorProfilerInfo->GetModuleInfo(modId,
                                                NULL,
                                                STRING_LENGTH,
                                                &nameLength,
                                                moduleFullName,
                                                &assemID);
    if (FAILED(hr))
    {
        printf("GetModuleInfo failed with hr=0x%x\n", hr);
        return WSTR("Unknown");
    }

    WCHAR *ptr = NULL;
    WCHAR *index = moduleFullName;
    // Find the last occurence of the \ character
    while (*index != 0)
    {
        if (*index == '\\' || *index == '/')
        {
            ptr = index;
        }

        ++index;
    }

    if (ptr == NULL)
    {
        return moduleFullName;
    }
    // Skip the last \ in the string
    ++ptr;

    WSTRING moduleName;
    while (*ptr != 0)
    {
        moduleName += *ptr;
        ++ptr;
    }

    return moduleName;
}


WSTRING Sampler::GetClassName(ClassID classId)
{
    ModuleID modId;
    mdTypeDef classToken;
    ClassID parentClassID;
    ULONG32 nTypeArgs;
    ClassID typeArgs[SHORT_LENGTH];
    HRESULT hr = S_OK;

    if (classId == NULL)
    {
        printf("NULL classId passed to GetClassName\n");
        return WSTR("Unknown");
    }

    hr = pCorProfilerInfo->GetClassIDInfo2(classId,
                                &modId,
                                &classToken,
                                &parentClassID,
                                SHORT_LENGTH,
                                &nTypeArgs,
                                typeArgs);
    if (CORPROF_E_CLASSID_IS_ARRAY == hr)
    {
        // We have a ClassID of an array.
        return WSTR("ArrayClass");
    }
    else if (CORPROF_E_CLASSID_IS_COMPOSITE == hr)
    {
        // We have a composite class
        return WSTR("CompositeClass");
    }
    else if (CORPROF_E_DATAINCOMPLETE == hr)
    {
        // type-loading is not yet complete. Cannot do anything about it.
        return WSTR("DataIncomplete");
    }
    else if (FAILED(hr))
    {
        printf("GetClassIDInfo returned hr=0x%x for classID=0x%" PRIx64 "\n", hr, (uint64_t)classId);
        return WSTR("Unknown");
    }

    COMPtrHolder<IMetaDataImport> pMDImport;
    hr = pCorProfilerInfo->GetModuleMetaData(modId,
                                            (ofRead | ofWrite),
                                            IID_IMetaDataImport,
                                            (IUnknown **)&pMDImport );
    if (FAILED(hr))
    {
        printf("GetModuleMetaData failed with hr=0x%x\n", hr);
        return WSTR("Unknown");
    }


    WCHAR wName[LONG_LENGTH];
    DWORD dwTypeDefFlags = 0;
    hr = pMDImport->GetTypeDefProps(classToken,
                                    wName,
                                    LONG_LENGTH,
                                    NULL,
                                    &dwTypeDefFlags,
                                    NULL);
    if (FAILED(hr))
    {
        printf("GetTypeDefProps failed with hr=0x%x\n", hr);
        return WSTR("Unknown");
    }

    WSTRING name = GetModuleName(modId);
    name += WSTR(" ");
    name += wName;

    if (nTypeArgs > 0)
    {
        name += WSTR("<");
    }

    for(ULONG32 i = 0; i < nTypeArgs; i++)
    {
        name += GetClassName(typeArgs[i]);

        if ((i + 1) != nTypeArgs)
        {
            name += WSTR(", ");
        }
    }

    if (nTypeArgs > 0)
    {
        name += WSTR(">");
    }

    return name;
}

WSTRING Sampler::GetFunctionName(FunctionID funcID, const COR_PRF_FRAME_INFO frameInfo)
{
    if (funcID == NULL)
    {
        return WSTR("Unknown_Native_Function");
    }

    ClassID classId = NULL;
    ModuleID moduleId = NULL;
    mdToken token = NULL;
    ULONG32 nTypeArgs = NULL;
    ClassID typeArgs[SHORT_LENGTH];

    HRESULT hr = pCorProfilerInfo->GetFunctionInfo2(funcID,
                                                   frameInfo,
                                                   &classId,
                                                   &moduleId,
                                                   &token,
                                                   SHORT_LENGTH,
                                                   &nTypeArgs,
                                                   typeArgs);
    if (FAILED(hr))
    {
        printf("GetFunctionInfo2 failed with hr=0x%x\n", hr);
    }

    COMPtrHolder<IMetaDataImport> pIMDImport;
    hr = pCorProfilerInfo->GetModuleMetaData(moduleId,
                                            ofRead,
                                            IID_IMetaDataImport,
                                            (IUnknown **)&pIMDImport);
    if (FAILED(hr))
    {
        printf("GetModuleMetaData failed with hr=0x%x\n", hr);
    }

    WCHAR funcName[STRING_LENGTH];
    hr = pIMDImport->GetMethodProps(token,
                                    NULL,
                                    funcName,
                                    STRING_LENGTH,
                                    0,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
    if (FAILED(hr))
    {
        printf("GetMethodProps failed with hr=0x%x\n", hr);
    }

    WSTRING name;

    // If the ClassID returned from GetFunctionInfo is 0, then the function is a shared generic function.
    if (classId != 0)
    {
        name += GetClassName(classId);
    }
    else
    {
        name += WSTR("SharedGenericFunction");
    }

    name += WSTR("::");

    name += funcName;

    // Fill in the type parameters of the generic method
    if (nTypeArgs > 0)
    {
        name += WSTR("<");
    }

    for(ULONG32 i = 0; i < nTypeArgs; i++)
    {
        name += GetClassName(typeArgs[i]);

        if ((i + 1) != nTypeArgs)
        {
            name += WSTR(", ");
        }
    }

    if (nTypeArgs > 0)
    {
        name += WSTR(">");
    }

    return name;
}

// static
void Sampler::DoSampling(Sampler *sampler, ICorProfilerInfo10 *pProfInfo, CorProfiler *parent)
{
    pProfInfo->InitializeCurrentThread();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        s_waitEvent.Wait();

        // This is a hack that was convenient for writing this profiler.
        // It checks if any methods have been jitted yet, but the runtime
        // can execute managed code from ready to run images without jitting
        // any code so it won't always work correctly.
        //
        // It also isn't strictly necessary to check this, you can suspend the
        // runtime at any point but for this profiler we don't care until there
        // are managed callstacks to sample.
        if (!parent->IsRuntimeExecutingManagedCode())
        {
            printf("Runtime has not started executing managed code yet.\n");
            continue;
        }

        if (!sampler->BeforeSampleAllThreads())
        {
            continue;
        }

        ICorProfilerThreadEnum* threadEnum = nullptr;
        HRESULT hr = pProfInfo->EnumThreads(&threadEnum);
        if (FAILED(hr))
        {
            printf("Error getting thread enumerator\n");
            continue;
        }

        ThreadID threadID;
        ULONG numReturned;
        while ((hr = threadEnum->Next(1, &threadID, &numReturned)) == S_OK)
        {
            printf("Starting stack walk for managed thread id=0x%" PRIx64 "\n", (uint64_t)threadID);

            if (!sampler->SampleThread(threadID))
            {
                continue;
            }

            printf("Ending stack walk for managed thread id=0x%" PRIx64 "\n", (uint64_t)threadID);
        }

        if (!sampler->AfterSampleAllThreads())
        {
            continue;
        }
    }
}

Sampler::Sampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent) :
    m_workerThread(DoSampling, this, pProfInfo, parent),
    pCorProfilerInfo(pProfInfo)
{

}


void Sampler::Start()
{
    s_waitEvent.Signal();
}

void Sampler::Stop()
{
    s_waitEvent.Reset();
}
