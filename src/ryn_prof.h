/*
ryn_prof v0.02 - A simple, recursive profiler. https://github.com/vitreousoul/ryn

Written while following the "Performance-Aware Programming Series" at https://www.computerenhance.com/

Version Log:
    v0.02 Add memory bandwidth measurement, remove numeric typedefs
    v0.01 Initial version

License:
    Permission to use, copy, modify, and/or distribute this software for
    any purpose with or without fee is hereby granted.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
    FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
    DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
    AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
    OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
Example:

#include "ryn_prof.h"

typedef enum
{
    TB_foo,
    TB_case_1,
    TB_case_2,
} timed_block_kind;

void Foo(int I)
{
    int J, K, X = 0;
    if (I < 50000)
    {
        BEGIN_TIMED_BLOCK(TB_case_1);
        for (J = 0; J < 10000; J++)
        {
            X += 1;
        }
        END_TIMED_BLOCK(TB_case_1);
    }
    else
    {
        BEGIN_TIMED_BLOCK(TB_case_2);
        for (K = 0; K < 10000; K++)
        {
            X += 1;
        }
        END_TIMED_BLOCK(TB_case_2);
    }
}

int main(void)
{
    int I;
    BeginProfile();
    BEGIN_TIMED_BLOCK(TB_foo);
    for (I = 0; I < 80000; I++)
    {
        Foo(I);
    }
    END_TIMED_BLOCK(TB_foo);
    EndAndPrintProfile();
    return 0;
}

*/

/* TODO: include windows code-paths */
/* TODO: prefix external names with "ryn" */
#include <stdio.h>
#include <x86intrin.h>
#include <sys/time.h>

#ifndef PROFILER
#define PROFILER 1
#endif

#define ArrayCount(a) ((sizeof(a))/(sizeof((a)[0])))

uint64_t ReadCPUTimer(void);
uint64_t ReadOSTimer(void);
void BeginProfile(void);
void EndAndPrintProfile(void);

inline uint64_t ReadCPUTimer(void)
{
    return __rdtsc();
}

static uint64_t GetOSTimerFreq(void)
{
    return 1000000;
}

uint64_t ReadOSTimer(void)
{
    struct timeval Value;
    gettimeofday(&Value, 0);
    uint64_t Result = GetOSTimerFreq()*(uint64_t)Value.tv_sec + (uint64_t)Value.tv_usec;
    return Result;
}

static int EstimateCpuFrequency()
{
    uint64_t MillisecondsToWait = 100;
    uint64_t OSFreq = GetOSTimerFreq();
    uint64_t CPUStart = ReadCPUTimer();
    uint64_t OSStart = ReadOSTimer();
    uint64_t OSEnd = 0;
    uint64_t OSElapsed = 0;
    uint64_t OSWaitTime = OSFreq * MillisecondsToWait / 1000;
    while(OSElapsed < OSWaitTime)
    {
        OSEnd = ReadOSTimer();
        OSElapsed = OSEnd - OSStart;
    }
    uint64_t CPUEnd = ReadCPUTimer();
    uint64_t CPUElapsed = CPUEnd - CPUStart;
    uint64_t CPUFreq = 0;
    if(OSElapsed)
    {
        CPUFreq = OSFreq * CPUElapsed / OSElapsed;
    }
    return CPUFreq;
}

#if PROFILER

/* TODO: check that registered timer indices are between 0-MAX_TIMERS */
#define MAX_TIMERS 1024

typedef struct
{
    uint64_t ElapsedExclusive;
    uint64_t ElapsedInclusive;
    uint64_t HitCount;
    uint64_t ProcessedByteCount;
    char *Label;
} timer_data;

typedef struct
{
    timer_data Timers[MAX_TIMERS];
    uint64_t StartTime;
    uint64_t EndTime;
} profiler;
static profiler GlobalProfiler;
uint32_t GlobalActiveTimer;

/* NOTE: We add 1 to TimerKey in order to reserve the 0 index for the root timer. */
#define GET_TIMER_BY_KEY(TimerKey) (GlobalProfiler.Timers[(TimerKey) + 1])

#define _BEGIN_TIMED_BLOCK(TimerKey, TargetTimer, ByteCount)            \
    uint32_t ParentTimer##TimerKey = GlobalActiveTimer;                      \
    GET_TIMER_BY_KEY(TimerKey).Label = #TimerKey;                       \
    GET_TIMER_BY_KEY(TimerKey).ProcessedByteCount += ByteCount;         \
    uint64_t OldTSCElapsedInclusive##TimerKey = GET_TIMER_BY_KEY(TimerKey).ElapsedInclusive; \
    TargetTimer = TimerKey + 1;                                         \
    uint64_t StartTime##TimerKey = ReadCPUTimer();

#define _END_TIMED_BLOCK(TimerKey, TargetTimer)                         \
    uint64_t Elapsed##TimerKey = ReadCPUTimer() - StartTime##TimerKey;       \
    TargetTimer = ParentTimer##TimerKey;                                \
    GlobalProfiler.Timers[ParentTimer##TimerKey].ElapsedExclusive -= Elapsed##TimerKey; \
    GET_TIMER_BY_KEY(TimerKey).ElapsedExclusive += Elapsed##TimerKey;   \
    GET_TIMER_BY_KEY(TimerKey).ElapsedInclusive = OldTSCElapsedInclusive##TimerKey + Elapsed##TimerKey; \
    GET_TIMER_BY_KEY(TimerKey).HitCount += 1

#define BEGIN_BANDWIDTH_BLOCK(TimerKey, ByteCount) _BEGIN_TIMED_BLOCK(TimerKey, GlobalActiveTimer, ByteCount)
#define BEGIN_TIMED_BLOCK(TimerKey) _BEGIN_TIMED_BLOCK(TimerKey, GlobalActiveTimer, 0)
#define END_TIMED_BLOCK(TimerKey) _END_TIMED_BLOCK(TimerKey, GlobalActiveTimer)

static void PrintTimeElapsed(uint64_t TotalElapsedTime, uint64_t CPUFreq, timer_data *Timer)
{
    double Percent = 100.0 * ((double)Timer->ElapsedExclusive / (double)TotalElapsedTime);
    printf("  %s[%llu]: %llu (%.2f%%", Timer->Label, Timer->HitCount, Timer->ElapsedExclusive, Percent);
    if(Timer->ElapsedInclusive != Timer->ElapsedExclusive)
    {
        double PercentWithChildren = 100.0 * ((double)Timer->ElapsedInclusive / (double)TotalElapsedTime);
        printf(", %.2f%% w/children", PercentWithChildren);
    }

    if(Timer->ProcessedByteCount)
    {
        double Megabyte = 1024.0f*1024.0f;
        double Gigabyte = Megabyte*1024.0f;

        double Seconds = (double)Timer->ElapsedInclusive / (double)CPUFreq;
        double BytesPerSecond = (double)Timer->ProcessedByteCount / Seconds;
        double Megabytes = (double)Timer->ProcessedByteCount / (double)Megabyte;
        double GigabytesPerSecond = BytesPerSecond / Gigabyte;

        printf("  %.3fmb at %.2fgb/s", Megabytes, GigabytesPerSecond);
    }
    printf(")\n");
}

void BeginProfile(void)
{
    GlobalProfiler.StartTime = ReadCPUTimer();
}

void EndProfile(void)
{
    GlobalProfiler.EndTime = ReadCPUTimer();
}

void EndAndPrintProfile(void)
{
    GlobalProfiler.EndTime = ReadCPUTimer();
    uint64_t CPUFreq = EstimateCpuFrequency();

    uint64_t TotalElapsedTime = GlobalProfiler.EndTime - GlobalProfiler.StartTime;

    if(CPUFreq)
    {
        float TotalElapsedTimeInMs = 1000.0 * (double)TotalElapsedTime / (double)CPUFreq;
        printf("\nTotal time: %0.4fms (CPU freq %llu)\n", TotalElapsedTimeInMs, CPUFreq);
    }

    for(uint32_t TimerIndex = 0; TimerIndex < ArrayCount(GlobalProfiler.Timers); ++TimerIndex)
    {
        timer_data *Timer = GlobalProfiler.Timers + TimerIndex;
        if(Timer->ElapsedInclusive)
        {
            PrintTimeElapsed(TotalElapsedTime, CPUFreq, Timer);
        }
    }
}

#else

typedef struct
{
    uint64_t StartTime;
    uint64_t EndTime;
} profiler;
static profiler GlobalProfiler;

#define BEGIN_TIMED_BLOCK(...)
#define END_TIMED_BLOCK(...)
#define BEGIN_TIMED_TIMER(...)
#define END_TIMED_TIMER(...)

void BeginProfile(void)
{
    GlobalProfiler.StartTime = ReadCPUTimer();
}

void EndProfile(void)
{
    GlobalProfiler.EndTime = ReadCPUTimer();
}

void EndAndPrintProfile()
{
    GlobalProfiler.EndTime = ReadCPUTimer();
    uint64_t CPUFreq = EstimateCpuFrequency();
    uint64_t TotalElapsedTime = GlobalProfiler.EndTime - GlobalProfiler.StartTime;
    if(CPUFreq)
    {
        float TotalElapsedTimeInMs = 1000.0 * (double)TotalElapsedTime / (double)CPUFreq;
        printf("\nTotal time: %0.4fms (CPU freq %llu)\n", TotalElapsedTimeInMs, CPUFreq);
    }
}

#endif
