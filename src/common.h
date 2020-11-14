// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <mutex>
#include <functional>
#include <condition_variable>
#include <string>

#if WIN32
#define WSTRING std::wstring
#define WSTR(str) L##str
#else // WIN32
#define WSTRING std::u16string
#define WSTR(str) u##str
#endif // WIN32

#define SHORT_LENGTH    32
#define STRING_LENGTH  256
#define LONG_LENGTH   1024

std::string ReadEnvironmentVariable(std::string name);

template <class MetaInterface>
class COMPtrHolder
{
public:
    COMPtrHolder()
    {
        m_ptr = NULL;
    }

    COMPtrHolder(MetaInterface* ptr)
    {
        if (ptr != NULL)
        {
            ptr->AddRef();
        }
        m_ptr = ptr;
    }

    ~COMPtrHolder()
    {
        if (m_ptr != NULL)
        {
            m_ptr->Release();
            m_ptr = NULL;
        }
    }
    MetaInterface* operator->()
    {
        return m_ptr;
    }

    MetaInterface** operator&()
    {
       // _ASSERT(m_ptr == NULL);
        return &m_ptr;
    }

    operator MetaInterface*()
    {
        return m_ptr;
    }
private:
    MetaInterface* m_ptr;
};

class ManualEvent
{
private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_set = false;

    static void DoNothing()
    {

    }

public:
    ManualEvent() = default;
    ~ManualEvent() = default;
    ManualEvent(ManualEvent& other) = delete;
    ManualEvent(ManualEvent&& other) = delete;
    ManualEvent& operator= (ManualEvent& other) = delete;
    ManualEvent& operator= (ManualEvent&& other) = delete;

    void Wait(std::function<void()> spuriousCallback = DoNothing)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        while (!m_set)
        {
            m_cv.wait(lock, [&]() { return m_set; });
            if (!m_set)
            {
                spuriousCallback();
            }
        }
    }

    void Signal()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_set = true;
    }

    void Reset()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_set = false;
    }
};
