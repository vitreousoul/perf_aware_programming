// TODO: include windows code-paths
#include <x86intrin.h>
#include <sys/time.h>

#ifndef PROFILER
#define PROFILER 1
#endif

u64 ReadCPUTimer(void);
u64 ReadOSTimer(void);
void begin_profile(void);
void end_and_print_profile(int pairs_count);

inline u64 ReadCPUTimer(void)
{
	return __rdtsc();
}

static u64 GetOSTimerFreq(void)
{
	return 1000000;
}

u64 ReadOSTimer(void)
{
	struct timeval Value;
	gettimeofday(&Value, 0);
	u64 Result = GetOSTimerFreq()*(u64)Value.tv_sec + (u64)Value.tv_usec;
	return Result;
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

#if PROFILER

#define MAX_TIMERS 1024
#define CSV 0
#define HEADER 0

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
	Timer_free,
	Timer_BEGIN_TIMED_TIMER,
	Timer_END_TIMED_TIMER,
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
Timer global_timed_timer;

#define _BEGIN_TIMED_BLOCK(tk, target_timer)											\
	Timer parent_timer##tk = global_active_timer;						\
	global_profiler.timers[(tk)].label = #tk;							\
	u64 OldTSCElapsedInclusive##tk = (global_profiler.timers + tk)->elapsed_inclusive; \
	target_timer = tk;											\
	u64 start_time##tk = ReadCPUTimer();

#define _END_TIMED_BLOCK(tk, target_timer)												\
	u64 elapsed##tk = ReadCPUTimer() - start_time##tk;					\
	target_timer = parent_timer##tk;								\
	global_profiler.timers[parent_timer##tk].elapsed_exclusive -= elapsed##tk; \
	global_profiler.timers[(tk)].elapsed_exclusive += elapsed##tk;		\
	global_profiler.timers[(tk)].elapsed_inclusive = OldTSCElapsedInclusive##tk + elapsed##tk; \
	global_profiler.timers[(tk)].hit_count += 1

#define BEGIN_TIMED_BLOCK(tk) _BEGIN_TIMED_BLOCK(tk, global_active_timer)
#define END_TIMED_BLOCK(tk) _END_TIMED_BLOCK(tk, global_active_timer)

#define BEGIN_TIMED_TIMER(tk) _BEGIN_TIMED_BLOCK(tk, global_timed_timer)
#define END_TIMED_TIMER(tk) _END_TIMED_BLOCK(tk, global_timed_timer)

void print_timer_stats(void);

static void print_time_elapsed(u64 total_elapsed_time, Timer_Data *timer)
{
	f64 percent = 100.0 * ((f64)timer->elapsed_exclusive / (f64)total_elapsed_time);
	printf("  %s[%llu]: %llu (%.2f%%", timer->label, timer->hit_count, timer->elapsed_exclusive, percent);
	if(timer->elapsed_inclusive != timer->elapsed_exclusive)
	{
		f64 percent_with_children = 100.0 * ((f64)timer->elapsed_inclusive / (f64)total_elapsed_time);
		printf(", %.2f%% w/children", percent_with_children);
	}
	printf(")\n");
}

void begin_profile(void)
{
	global_profiler.start_time = ReadCPUTimer();
}

void end_and_print_profile(int pairs_count)
{
	global_profiler.end_time = ReadCPUTimer();
	u64 CPUFreq = estimate_cpu_frequency();

	u64 total_elapsed_time = global_profiler.end_time - global_profiler.start_time;

#if !CSV
	if(CPUFreq)
	{
		float total_elapsed_time_in_ms = 1000.0 * (f64)total_elapsed_time / (f64)CPUFreq;
		printf("\nTotal time: %0.4fms (CPU freq %llu)\n", total_elapsed_time_in_ms, CPUFreq);
		printf("    %0.4f pairs/ms\n", (f64)pairs_count / total_elapsed_time_in_ms);
	}

	for(u32 timer_index = 0; timer_index < array_count(global_profiler.timers); ++timer_index)
	{
		Timer_Data *timer = global_profiler.timers + timer_index;
		if(timer->elapsed_inclusive)
		{
			print_time_elapsed(total_elapsed_time, timer);
		}
	}
#else
#if HEADER
	for (u32 i = 0; i < array_count(global_profiler.timers); ++i)
	{
		Timer_Data timer = global_profiler.timers[i];
		if(timer.label) printf("%s,", timer.label);
	}
	printf("pairs_count\n");
#endif
	for (u32 i = 0; i < array_count(global_profiler.timers); ++i)
	{
		Timer_Data timer = global_profiler.timers[i];
		if(timer.label)
		{
			f64 percent = 100.0 * ((f64)timer.elapsed_exclusive / (f64)total_elapsed_time);
			printf("%f,", percent);
		}
	}
	printf("%d\n", pairs_count);
#endif
}

#else

typedef struct
{
	u64 start_time;
	u64 end_time;
} Profiler;
static Profiler global_profiler;

#define BEGIN_TIMED_BLOCK(...)
#define END_TIMED_BLOCK(...)
#define BEGIN_TIMED_TIMER(...)
#define END_TIMED_TIMER(...)

void begin_profile(void)
{
	global_profiler.start_time = ReadCPUTimer();
}

void end_and_print_profile(int pairs_count)
{
	global_profiler.end_time = ReadCPUTimer();
	u64 CPUFreq = estimate_cpu_frequency();
	u64 total_elapsed_time = global_profiler.end_time - global_profiler.start_time;
	if(CPUFreq)
	{
		float total_elapsed_time_in_ms = 1000.0 * (f64)total_elapsed_time / (f64)CPUFreq;
		printf("\nTotal time: %0.4fms (CPU freq %llu)\n", total_elapsed_time_in_ms, CPUFreq);
		printf("    %0.4f pairs/ms\n", (f64)pairs_count / total_elapsed_time_in_ms);
	}
}

#endif
