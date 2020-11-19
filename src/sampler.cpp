// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <thread>
#include <cstdio>
#include <cinttypes>

#include "CorProfiler.h"
#include "sampler.h"

ManualEvent Sampler::s_waitEvent;

WSTRING Sampler::GetModuleName(ModuleID modId)
{
    WCHAR moduleFullName[STRING_LENGTH];
    ULONG nameLength = 0;
    AssemblyID assemID;

    if (modId == NULL)
    {
        fprintf(m_outputFile, "NULL modId passed to GetModuleName\n");
        return WSTR("Unknown");
    }

    HRESULT hr = m_pCorProfilerInfo->GetModuleInfo(modId,
                                                NULL,
                                                STRING_LENGTH,
                                                &nameLength,
                                                moduleFullName,
                                                &assemID);
    if (FAILED(hr))
    {
        fprintf(m_outputFile, "GetModuleInfo failed with hr=0x%x\n", hr);
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
        fprintf(m_outputFile, "NULL classId passed to GetClassName\n");
        return WSTR("Unknown");
    }

    hr = m_pCorProfilerInfo->GetClassIDInfo2(classId,
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
        fprintf(m_outputFile, "GetClassIDInfo returned hr=0x%x for classID=0x%" PRIx64 "\n", hr, (uint64_t)classId);
        return WSTR("Unknown");
    }

    IMetaDataImport *pMDImport = m_parent->GetMetadataForModule(modId);
    if (pMDImport == NULL)
    {
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
        fprintf(m_outputFile, "GetTypeDefProps failed with hr=0x%x\n", hr);
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

    HRESULT hr = m_pCorProfilerInfo->GetFunctionInfo2(funcID,
                                                   frameInfo,
                                                   &classId,
                                                   &moduleId,
                                                   &token,
                                                   SHORT_LENGTH,
                                                   &nTypeArgs,
                                                   typeArgs);
    if (FAILED(hr))
    {
        fprintf(m_outputFile, "GetFunctionInfo2 failed with hr=0x%x\n", hr);
    }

    IMetaDataImport *pMDImport = m_parent->GetMetadataForModule(moduleId);
    if (pMDImport == NULL)
    {
        return WSTR("Unknown");
    }

    WCHAR funcName[STRING_LENGTH];
    hr = pMDImport->GetMethodProps(token,
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
        fprintf(m_outputFile, "GetMethodProps failed with hr=0x%x\n", hr);
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
void Sampler::DoSampling(Sampler *sampler, ICorProfilerInfo10 *pProfInfo, CorProfiler *parent, FILE *outputFile)
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
            fprintf(outputFile, "Runtime has not started executing managed code yet.\n");
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
            fprintf(outputFile, "Error getting thread enumerator\n");
            continue;
        }

        ThreadID threadID;
        ULONG numReturned;
        while ((hr = threadEnum->Next(1, &threadID, &numReturned)) == S_OK)
        {
            fprintf(outputFile, "Starting stack walk for managed thread id=0x%" PRIx64 "\n", (uint64_t)threadID);

            sampler->SampleThread(threadID);

            fprintf(outputFile, "Ending stack walk for managed thread id=0x%" PRIx64 "\n", (uint64_t)threadID);
        }

        if (!sampler->AfterSampleAllThreads())
        {
            continue;
        }
    }
}

Sampler::Sampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent) :
    m_workerThread(),
    m_pCorProfilerInfo(pProfInfo),
    m_parent(parent),
    m_outputFile(NULL),
    m_threadIDMap()
{
    std::string fileName = std::tmpnam(nullptr) + std::string(".txt");
    m_outputFile = fopen(fileName.c_str(), "w+");
    printf("Writing sampler output to \"%s\"\n", fileName.c_str());
    m_workerThread = std::thread(DoSampling, this, pProfInfo, parent, m_outputFile);
}

Sampler::~Sampler()
{
    fclose(m_outputFile);
}


void Sampler::Start()
{
    s_waitEvent.Signal();
}

void Sampler::Stop()
{
    s_waitEvent.Reset();
}

void Sampler::ThreadCreated(ThreadID threadId)
{
    NativeThreadInfo nativeThreadInfo;
    nativeThreadInfo.pThreadID = GetCurrentPThreadID();
    nativeThreadInfo.threadID = GetCurrentNativeThreadID();
    nativeThreadInfo.stackBase = GetCurrentThreadStackBase();

    m_threadIDMap.insertNew(threadId, nativeThreadInfo);
}

void Sampler::ThreadDestroyed(ThreadID threadId)
{
    // should probably delete it from the map
}

pthread_t Sampler::GetCurrentPThreadID()
{
    return pthread_self();
}

pthread_t Sampler::GetPThreadID(ThreadID threadID)
{
    auto it = m_threadIDMap.find(threadID);
    if (it == m_threadIDMap.end())
    {
        assert(!"Unexpected thread found");
        return 0;
    }

    pthread_t pThreadId = it->second.pThreadID;
    return pThreadId;
}

NativeThreadID Sampler::GetNativeThreadID(ThreadID threadID)
{
    auto it = m_threadIDMap.find(threadID);
    if (it == m_threadIDMap.end())
    {
        assert(!"Unexpected thread found");
        return 0;
    }

    NativeThreadID nativeThreadId = it->second.threadID;
    return nativeThreadId;
}


void * Sampler::GetStackBase(ThreadID threadID)
{
    auto it = m_threadIDMap.find(threadID);
    if (it == m_threadIDMap.end())
    {
        assert(!"Unexpected thread found");
        return 0;
    }

    void *stackBase = it->second.stackBase;
    return stackBase;
}
