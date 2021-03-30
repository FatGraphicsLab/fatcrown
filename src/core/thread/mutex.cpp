/*
 * Copyright (C) 2021-2077 FATCROWN Team.
 * License: https://github.com/FatGraphicsLab/fatcrown/blob/main/LICENSE
 *
 * @author   kasicass@gmail.com
 * @date     2021-03-30
 */

#include "core/error/error.inl"
#include "core/thread/mutex.h"
#include <new>

#if CROWN_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace crown
{

struct Private
{
    CRITICAL_SECTION cs;
};

Mutex::Mutex()
{
    CE_STATIC_ASSERT(sizeof(_data) >= sizeof(*_priv));
    _priv = new (_data) Private();

    InitializeCriticalSection(&_priv->cs);
}

Mutex::~Mutex()
{
    DeleteCriticalSection(&_priv->cs);

    _priv->~Private();
}

void Mutex::lock()
{
    EnterCriticalSection(&_priv->cs);
}

void Mutex::unlock()
{
    LeaveCriticalSection(&_priv->cs);
}

void* Mutex::native_handle()
{
    return &_priv->cs;
}

} // namespacr crown

#else

#include <pthread.h>

namespace crown
{

struct Private
{
    pthread_mutex_t mutex;
};

Mutex::Mutex()
{
    CE_STATIC_ASSERT(sizeof(_data) >= sizeof(*_priv));
    _priv = new (_data) Private();

    pthread_mutexattr_t attr;
    int err = pthread_mutexattr_init(&attr);
    CE_ASSERT(err == 0, "pthread_mutexattr_init: errno = %d", err);
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    CE_ASSERT(err == 0, "pthread_mutexattr_settype: errno = %d", err);
    err = pthread_mutex_init(&_priv->mutex, &attr);
    CE_ASSERT(err == 0, "pthread_mutex_init: errno = %d", err);
    err = pthread_mutexattr_destroy(&attr);
    CE_ASSERT(err == 0, "pthread_mutexattr_destroy: errno = %d", err);
    CE_UNUSED(err);
}

Mutex::~Mutex()
{
    int err = pthread_mutex_destroy(&_priv->mutex);
    CE_ASSERT(err == 0, "pthread_mutex_destroy: errno = %d", err);
    CE_UNUSED(err);

    _priv->~Private();
}

void Mutex::lock()
{
    int err = pthread_mutex_lock(&_priv->mutex);
    CE_ASSERT(err == 0, "pthread_mutex_lock: errno = %d", err);
    CE_UNUSED(err);
}

void Mutex::unlock()
{
    int err = pthread_mutex_unlock(&_priv->mutex);
    CE_ASSERT(err == 0, "pthread_mutex_unlock: errno = %d", err);
    CE_UNUSED(err);
}

void* Mutex::native_handle()
{
    return &_priv->mutex;
}

} // namespacr crown

#endif
