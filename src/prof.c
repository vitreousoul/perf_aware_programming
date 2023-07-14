#include <x86intrin.h>
#include <sys/time.h>

#define MAX_TIMERS 1024

typedef enum
{
	Timer_ROOT,
	Timer_parse,
	Timer_parse_read,
	Timer_parse_parse,
	Timer_parse_json_value,
	Timer_parse_json_array,
	Timer_parse_json_object,
	Timer_parse_json_string,
	Timer_parse_json_digit,
	Timer_json_array_push,
	Timer_json_object_push,
	Timer_chomp_space,
	Timer_process,
} Timer;

typedef struct
{
	u64 elapsed_exclusive;
	u64 elapsed_inclusive;
	u64 hit_count;
	char *label;
} Timer_Data;

typedef struct
{
	Timer_Data timers[MAX_TIMERS];
	u64 start_time;
	u64 end_time;
} Profiler;
static Profiler global_profiler;
Timer global_active_timer;

#define BEGIN_TIMED_BLOCK(tk)											\
	Timer parent_timer##tk = global_active_timer;						\
	global_profiler.timers[(tk)].label = #tk;							\
	u64 OldTSCElapsedInclusive##tk = (global_profiler.timers + tk)->elapsed_inclusive; \
	global_active_timer = tk;											\
	u64 start_time##tk = ReadCPUTimer();

#define END_TIMED_BLOCK(tk)												\
	u64 elapsed##tk = ReadCPUTimer() - start_time##tk;					\
	global_active_timer = parent_timer##tk;								\
	global_profiler.timers[parent_timer##tk].elapsed_exclusive -= elapsed##tk; \
	global_profiler.timers[(tk)].elapsed_exclusive += elapsed##tk;		\
	global_profiler.timers[(tk)].elapsed_inclusive = OldTSCElapsedInclusive##tk + elapsed##tk; \
	global_profiler.timers[(tk)].hit_count += 1

u64 ReadOSTimer(void);
u64 ReadCPUTimer(void);
void print_timer_stats(void);
void begin_profile(void);
void end_and_print_profile(void);

static u64 GetOSTimerFreq(void)
{
	return 1000000;
}

u64 ReadOSTimer(void)
{
	// NOTE(casey): The "struct" keyword is not necessary here when compiling in C++,
	// but just in case anyone is using this file from C, I include it.
	struct timeval Value;
	gettimeofday(&Value, 0);

	u64 Result = GetOSTimerFreq()*(u64)Value.tv_sec + (u64)Value.tv_usec;
	return Result;
}

/* NOTE(casey): This does not need to be "inline", it could just be "static"
   because compilers will inline it anyway. But compilers will warn about
   static functions that aren't used. So "inline" is just the simplest way
   to tell them to stop complaining about that. */
inline u64 ReadCPUTimer(void)
{
	// NOTE(casey): If you were on ARM, you would need to replace __rdtsc
	// with one of their performance counter read instructions, depending
	// on which ones are available on your platform.

	return __rdtsc();
}

static int estimate_cpu_frequency()
{
	u64 MillisecondsToWait = 100;
	u64 OSFreq = GetOSTimerFreq();
	u64 CPUStart = ReadCPUTimer();
	u64 OSStart = ReadOSTimer();
	u64 OSEnd = 0;
	u64 OSElapsed = 0;
	u64 OSWaitTime = OSFreq * MillisecondsToWait / 1000;
	while(OSElapsed < OSWaitTime)
	{
		OSEnd = ReadOSTimer();
		OSElapsed = OSEnd - OSStart;
	}
	u64 CPUEnd = ReadCPUTimer();
	u64 CPUElapsed = CPUEnd - CPUStart;
	u64 CPUFreq = 0;
	if(OSElapsed)
	{
		CPUFreq = OSFreq * CPUElapsed / OSElapsed;
	}
	return CPUFreq;
}

static void print_time_elapsed(u64 total_elapsed_time, Timer_Data *timer)
{
	f64 Percent = 100.0 * ((f64)timer->elapsed_exclusive / (f64)total_elapsed_time);
	printf("  %s[%llu]: %llu (%.2f%%", timer->label, timer->hit_count, timer->elapsed_exclusive, Percent);
	if(timer->elapsed_inclusive != timer->elapsed_exclusive)
	{
		f64 PercentWithChildren = 100.0 * ((f64)timer->elapsed_inclusive / (f64)total_elapsed_time);
		printf(", %.2f%% w/children", PercentWithChildren);
	}
	printf(")\n");
}

void begin_profile(void)
{
	global_profiler.start_time = ReadCPUTimer();
}

void end_and_print_profile(void)
{
	global_profiler.end_time = ReadCPUTimer();
	u64 CPUFreq = estimate_cpu_frequency();

	u64 TotalCPUElapsed = global_profiler.end_time - global_profiler.start_time;

	if(CPUFreq)
	{
		printf("\nTotal time: %0.4fms (CPU freq %llu)\n", 1000.0 * (f64)TotalCPUElapsed / (f64)CPUFreq, CPUFreq);
	}

	for(u32 AnchorIndex = 0; AnchorIndex < array_count(global_profiler.timers); ++AnchorIndex)
	{
		Timer_Data *timer = global_profiler.timers + AnchorIndex;
		if(timer->elapsed_inclusive)
		{
			print_time_elapsed(TotalCPUElapsed, timer);
		}
	}
}
