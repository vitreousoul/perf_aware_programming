#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef uint32_t u32;
typedef uint64_t u64;
typedef double f64;

#include "haversine_process.h"
#include "prof.c"
#include "json_parser.h"
#include "haversine_generate_json.c"


typedef enum
{
    Haversine_Process_Mode_NONE,
    Haversine_Process_Mode_Generate,
    Haversine_Process_Mode_Process,
} Haversine_Process_Mode;

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

typedef struct
{
    float average;
    int pairs_count;
} haversine_result;
static haversine_result process_haversine_json(Json_Value *object)
{
    float sum = 0.0f;
    int count = 0;
    haversine_result result;
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
    result.pairs_count = count;
    result.average = count == 0 ? 0 : sum / (float)count;
    return result;
}

int main(int arg_count, char **args)
{
    begin_profile();
    if (arg_count < 2)
    {
        print_usage();
        return 1;
    }
    char *mode = args[1];
    if (string_match(mode, "process"))
    {
        Json_Value *value = parse_json("../dist/haversine.json");
        BEGIN_TIMED_BLOCK(Timer_process);
        haversine_result result = process_haversine_json(value);
        END_TIMED_BLOCK(Timer_process);
        Json_Value *average_check = object_lookup(value, "average");
        BEGIN_TIMED_BLOCK(Timer_free);
        free_json(value);
        END_TIMED_BLOCK(Timer_free);
        printf("global_malloc_count %llu\n", global_malloc_count);
        printf("global_free_count %llu\n", global_free_count);
#if !CSV
        if (average_check && average_check->kind == Json_Value_Kind_Float)
        {
             printf("error %f\n", result.average - average_check->number_float);
        }
#endif
        end_and_print_profile(result.pairs_count);
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
        write_haversine_json(file_path, seed, pairs_count);
    }
    return 0;
}
