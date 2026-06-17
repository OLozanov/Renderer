#pragma once

#include <thread>
#include "stdint.h"

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

#undef min
#undef max

#elif defined(__linux__)

#include <pthread.h>
#include <sched.h>

#elif defined(__APPLE__)

#include <pthread.h>
#include <mach/thread_act.h>
#include <mach/thread_policy.h>

#endif

inline bool SetThreadAffinity(std::thread::native_handle_type thread, uint32_t core) 
{
#if defined(_WIN32) || defined(_WIN64)

    DWORD_PTR result = SetThreadAffinityMask(thread, static_cast<DWORD_PTR>(1 << core));
    return result != 0;

#elif defined(__linux__)

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    return rc == 0;

#elif defined(__APPLE__)

    thread_affinity_policy_data_t policy = { core };
    mach_port_t mach_thread = pthread_mach_thread_np(thread);
    int rc = thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
    return rc == KERN_SUCCESS;

#else
    // Unsupported platforms
    return false;
#endif
}