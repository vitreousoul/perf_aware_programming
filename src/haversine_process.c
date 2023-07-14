#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef uint64_t u64;
typedef double f64;

#include "json_parser.h"
#include "haversine_generate_json.c"

#include "prof.c"

typedef enum
{
    Haversine_Process_Mode_NONE,
    Haversine_Process_Mode_Generate,
    Haversine_Process_Mode_Process,
} Haversine_Process_Mode;

u64 timers[64];

static int string_match(char *string_a, char *string_b)
{
    int i = 0, result = 1;
    if (!string_a || !string_b) return 0;
    while(1)
    {
        int null_a = string_a[i] == 0;
        int null_b = string_b[i] == 0;
        if (string_a[i] != string_b[i])
        {
            result = 0;
            break;
        }
        if (null_a || null_b)
        {
            break;
        }
        i += 1;
    }
    return result;
}

static void print_usage()
{
    printf("Usage:    ./test.sh generate [random_seed] [number_of_pairs]\n");
    printf("          ./test.sh process\n");
    printf("Examples: ./test.sh generate 123456 1000\n");
    printf("          ./test.sh process\n");
}

static Json_Value *object_lookup(Json_Value *object, char *key)
{
    int i;
    for (i = 0; i < object->object->used; ++i)
    {
        if (string_match(key, object->object->key[i]))
        {
            return object->object->value[i];
        }
    }
    return 0;
}

static float process_haversine_json(Json_Value *object)
{
    float sum = 0.0f;
    int count = 0;
    if (object && object->kind == Json_Value_Kind_Object)
    {
        Json_Value *pairs_array_value = object_lookup(object, "pairs");
        Json_Array *pairs_array = pairs_array_value->array;
        if (pairs_array_value && pairs_array_value->kind == Json_Value_Kind_Array)
        {
            while (pairs_array)
            {
                int i;
                for (i = 0; i < pairs_array->used; ++i)
                {
                    Json_Value *x0 = object_lookup(pairs_array->value[i], "x0");
                    Json_Value *y0 = object_lookup(pairs_array->value[i], "y0");
                    Json_Value *x1 = object_lookup(pairs_array->value[i], "x1");
                    Json_Value *y1 = object_lookup(pairs_array->value[i], "y1");
                    if (!(x0 && y0 && x1 && y1))
                    {
                        printf("process_haversine_json null x/y value\n");
                    }
                    float haversine = haversine_of_degrees(x0->number_float, y0->number_float, x1->number_float, y1->number_float, EARTH_RADIUS_KM);
                    sum += haversine;
                    count += 1;
                }
                pairs_array = pairs_array->next;
            }
        }
        else
        {
            printf("process_haversine_json expected array\n");
        }
    }
    else
    {
        printf("process_haversine_json expected object\n");
    }
    float result = count == 0 ? 0 : sum / (float)count;
    return result;
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

    /* printf("    OS Freq: %llu (reported)\n", OSFreq); */
    /* printf("   OS Timer: %llu -> %llu = %llu elapsed\n", OSStart, OSEnd, OSElapsed); */
    /* printf(" OS Seconds: %.4f\n", (f64)OSElapsed/(f64)OSFreq); */
    /* printf("  CPU Timer: %llu -> %llu = %llu elapsed\n", CPUStart, CPUEnd, CPUElapsed); */
    /* printf("   CPU Freq: %llu (guessed)\n", CPUFreq); */

    return CPUFreq;
}

int main(int arg_count, char **args)
{
    u64 cpu_freqency = estimate_cpu_frequency();
    timers[0] = ReadCPUTimer();
    if (arg_count < 2)
    {
        print_usage();
        return 1;
    }
    char *mode = args[1];
    if (string_match(mode, "process"))
    {

        timers[1] = ReadCPUTimer();
        Json_Value *value = parse_json("../dist/haversine.json");
        timers[2] = ReadCPUTimer();
        float average = process_haversine_json(value);
        timers[3] = ReadCPUTimer();
        /* printf("average %f\n", average); */
        Json_Value *average_check = object_lookup(value, "average");
        if (average_check && average_check->kind == Json_Value_Kind_Float)
        {
            printf("error %f\n", average - average_check->number_float);
        }
        float total_time = (float)timers[3] - (float)timers[0];
        printf("  cpu freq: %llu\n", cpu_freqency);
        printf("   startup: %llu %.2f%%\n", timers[1] - timers[0], 100.0f * (float)(timers[1] - timers[0]) / total_time);
        printf("     parse: %llu %.2f%%\n", timers[2] - timers[1], 100.0f * (float)(timers[2] - timers[1]) / total_time);
        printf("   process: %llu %.2f%%\n", timers[3] - timers[2], 100.0f * (float)(timers[3] - timers[2]) / total_time);
        printf("total time: %.2f seconds\n", total_time / (float)cpu_freqency);
    }
    else if (string_match(mode, "generate"))
    {
        if (arg_count < 4)
        {
            print_usage();
            return 1;
        }
        int seed = atoi(args[2]);
        int pairs_count = atoi(args[3]);
        char *file_path = "../dist/haversine.json";
        float average = write_haversine_json(file_path, seed, pairs_count);
        /* printf("average %f\n", average); */
    }
    return 0;
}
