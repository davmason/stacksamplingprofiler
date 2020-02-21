
#include "CorProfiler.h"
#include "sampler.h"

using std::wstring;

ManualEvent Sampler::s_waitEvent;
Sampler *Sampler::s_instance = nullptr;

HRESULT __stdcall DoStackSnapshotStackSnapShotCallbackWrapper(
    FunctionID funcId,
    UINT_PTR ip,
    COR_PRF_FRAME_INFO frameInfo,
    ULONG32 contextSize,
    BYTE context[],
    void* clientData)
{
    return Sampler::Instance()->StackSnapshotCallback(funcId,
        ip,
        frameInfo,
        contextSize,
        context,
        clientData);
}

Sampler::Sampler(ICorProfilerInfo10* pProfInfo, CorProfiler *parent) :
    m_workerThread(DoSampling, pProfInfo, parent)
{
    Sampler::s_instance = this;
}

// static
void Sampler::DoSampling(ICorProfilerInfo10 *pProfInfo, CorProfiler *parent)
{
    Sampler::Instance()->corProfilerInfo = parent->corProfilerInfo;

    pProfInfo->InitializeCurrentThread();

    while (true)
    {
        Sleep(100);

        s_waitEvent.Wait();

        if (!parent->IsRuntimeExecutingManagedCode())
        {
            printf("Runtime has not started executing managed code yet.\n");
            continue;
        }

        printf("Suspending runtime\n");
        HRESULT hr = pProfInfo->SuspendRuntime();
        if (FAILED(hr))
        {
            printf("Error suspending runtime... hr=0x%x \n", hr);
            continue;
        }

        ICorProfilerThreadEnum* threadEnum = nullptr;
        hr = pProfInfo->EnumThreads(&threadEnum);
        if (FAILED(hr))
        {
            printf("Error getting thread enumerator\n");
            continue;
        }

        ThreadID threadID;
        ULONG numReturned;
        while ((hr = threadEnum->Next(1, &threadID, &numReturned)) == S_OK)
        {
            printf("Starting stack walk for managed thread id=0x%I64x\n", threadID);

            hr = pProfInfo->DoStackSnapshot(threadID,
                                            DoStackSnapshotStackSnapShotCallbackWrapper,
                                            COR_PRF_SNAPSHOT_REGISTER_CONTEXT,
                                            NULL,
                                            NULL,
                                            0);
            if (FAILED(hr))
            {
                if (hr == E_FAIL)
                {
                    printf("Managed thread id=0x%I64x has no managed frames to walk \n", threadID);
                }
                else
                {
                    printf("DoStackSnapshot for thread id=0x%I64x failed with hr=0x%x \n", threadID, hr);
                }
            }

            printf("Ending stack walk for managed thread id=0x%I64x\n", threadID);
        }

        printf("Resuming runtime\n");
        hr = pProfInfo->ResumeRuntime();
        if (FAILED(hr))
        {
            printf("ResumeRuntime failed with hr=0x%x \n", hr);
        }
    }
}

void Sampler::Start()
{
    s_waitEvent.Signal();
}

void Sampler::Stop()
{
    s_waitEvent.Reset();
}

HRESULT Sampler::StackSnapshotCallback(FunctionID funcId, UINT_PTR ip, COR_PRF_FRAME_INFO frameInfo, ULONG32 contextSize, BYTE context[], void* clientData)
{
    wstring functionName = GetFunctionName(funcId, frameInfo);
    wprintf(L"    %s (funcId=%I64u)\n", functionName.c_str(), funcId);

    return S_OK;
}

wstring Sampler::GetModuleName(ModuleID modId)
{
    WCHAR moduleFullName[STRING_LENGTH];
    ULONG nameLength = 0;
    AssemblyID assemID;

    if (modId == NULL)
    {
        printf("NULL modId passed to GetModuleName\n");
        return L"Unknown";
    }

    HRESULT hr = corProfilerInfo->GetModuleInfo(modId,
                                                NULL,
                                                STRING_LENGTH,
                                                &nameLength,
                                                moduleFullName,
                                                &assemID);
    if (FAILED(hr))
    {
        printf("GetModuleInfo failed with hr=0x%x\n", hr);
        return L"Unknown";
    }

    WCHAR *ptr = wcsrchr(moduleFullName, L'\\');
    if (ptr == NULL)
    {
        return moduleFullName;
    }
    // Skip the last \ in the string
    ++ptr;

    wstring moduleName;
    while (*ptr != 0)
    {
        moduleName += *ptr;
        ++ptr;
    }

    return moduleName;
}


wstring Sampler::GetClassName(ClassID classId)
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
        return L"Unknown";
    }

    hr = corProfilerInfo->GetClassIDInfo2(classId,
                                &modId,
                                &classToken,
                                &parentClassID,
                                SHORT_LENGTH,
                                &nTypeArgs,
                                typeArgs);
    if (CORPROF_E_CLASSID_IS_ARRAY == hr)
    {
        // We have a ClassID of an array.
        return L"ArrayClass";
    }
    else if (CORPROF_E_CLASSID_IS_COMPOSITE == hr)
    {
        // We have a composite class
        return L"CompositeClass";
    }
    else if (CORPROF_E_DATAINCOMPLETE == hr)
    {
        // type-loading is not yet complete. Cannot do anything about it.
        return L"DataIncomplete";
    }
    else if (FAILED(hr))
    {
        printf("GetClassIDInfo returned hr=0x%x for classID=0x%llx\n", hr, classId);
        return L"Unknown";
    }

    COMPtrHolder<IMetaDataImport> pMDImport;
    hr = corProfilerInfo->GetModuleMetaData(modId,
                                            (ofRead | ofWrite),
                                            IID_IMetaDataImport,
                                            (IUnknown **)&pMDImport );
    if (FAILED(hr))
    {
        printf("GetModuleMetaData failed with hr=0x%x\n", hr);
        return L"Unknown";
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
        return L"Unknown";
    }

    wstring name = GetModuleName(modId);
    name += L".";
    name += wName;

    if (nTypeArgs > 0)
    {
        name += L"<";
    }

    for(ULONG32 i = 0; i < nTypeArgs; i++)
    {
        name += GetClassName(typeArgs[i]);

        if ((i + 1) != nTypeArgs)
        {
            name += L", ";
        }
    }

    if (nTypeArgs > 0)
    {
        name += L">";
    }

    return name;
}

wstring Sampler::GetFunctionName(FunctionID funcID, const COR_PRF_FRAME_INFO frameInfo)
{
    if (funcID == NULL)
    {
        return L"Unknown_Native_Function";
    }

    ClassID classId = NULL;
    ModuleID moduleId = NULL;
    mdToken token = NULL;
    ULONG32 nTypeArgs = NULL;
    ClassID typeArgs[SHORT_LENGTH];

    HRESULT hr = corProfilerInfo->GetFunctionInfo2(funcID,
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
    hr = corProfilerInfo->GetModuleMetaData(moduleId,
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

    wstring name;

    // If the ClassID returned from GetFunctionInfo is 0, then the function is a shared generic function.
    if (classId != 0)
    {
        name += GetClassName(classId);
    }
    else
    {
        name += L"SharedGenericFunction";
    }

    name += L"::";

    name += funcName;

    // Fill in the type parameters of the generic method
    if (nTypeArgs > 0)
    {
        name += L"<";
    }

    for(ULONG32 i = 0; i < nTypeArgs; i++)
    {
        name += GetClassName(typeArgs[i]);

        if ((i + 1) != nTypeArgs)
        {
            name += L", ";
        }
    }

    if (nTypeArgs > 0)
    {
        name += L">";
    }

    return name;
}
