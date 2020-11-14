// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#pragma once

#include <mutex>
#include <shared_mutex>
#include <functional>
#include <condition_variable>
#include <string>
#include <set>
#include <map>

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

class AutoEvent
{
private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_set = false;

    static void DoNothing()
    {

    }

public:
    AutoEvent() = default;
    ~AutoEvent() = default;
    AutoEvent(AutoEvent& other) = delete;
    AutoEvent(AutoEvent &&other) = delete;
    AutoEvent &operator=(AutoEvent &other) = delete;
    AutoEvent &operator=(AutoEvent &&other) = delete;

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
        m_set = false;
    }

    void WaitFor(int milliseconds, std::function<void()> spuriousCallback = DoNothing)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        while (!m_set)
        {
            m_cv.wait_for(lock, std::chrono::milliseconds(milliseconds), [&]() { return m_set; });
            if (!m_set)
            {
                spuriousCallback();
            }
        }
        m_set = false;
    }

    void Signal()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_set = true;
        m_cv.notify_one();
    }
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


template<class Key, class Value>
class ThreadSafeMap
{
  private:
     std::map<Key, Value> _map;
     mutable std::shared_mutex _mutex;

  public:
    typename std::map<Key, Value>::const_iterator find(Key key) const
    {
        std::shared_lock lock(_mutex);
        return _map.find(key);
    }

    typename std::map<Key, Value>::const_iterator end() const
    {
        return _map.end();
    }

    // Returns true if new value was inserted
    bool insertNew(Key key, Value value)
    {
        std::unique_lock lock(_mutex);

        if (_map.find(key) != _map.end())
        {
            return false;
        }

        _map[key] = value;
        return true;
    }
};
