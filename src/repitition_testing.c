#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t s32;

#include "ryn_prof.h"

typedef enum
{
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

#include "json_parser.h"


static void test_read_file(char *file_path)
{
    uint64_t seconds_to_wait = 100;
    int cpu_freq = EstimateCpuFrequency();
    uint64_t os_wait_time = GetOSTimerFreq() * seconds_to_wait;
    uint64_t os_start_time = ReadOSTimer();
    uint64_t os_elapsed = 0;
    uint64_t minimum_runtime = 0xffffffff;

    struct stat stat_result;
    int stat_error = stat(file_path, &stat_result);
    if (stat_error)
    {
        printf("read_file stat error\n");
        return;
    }

    Json_Buffer *buffer = allocate_memory(sizeof(Json_Buffer));
    FILE *file = fopen(file_path, "rb");
    buffer->data = allocate_memory(stat_result.st_size + 1);

    printf("File size %lld bytes\n", stat_result.st_size);

    /* time the fread call */
    while(os_elapsed < os_wait_time)
    {
        uint64_t start_time = ReadCPUTimer();
        fread(buffer->data, 1, stat_result.st_size, file);
        uint64_t run_time = ReadCPUTimer() - start_time;
        if (run_time < minimum_runtime)
        {
            minimum_runtime = run_time;
            float seconds_elapsed = (float)minimum_runtime / (float)cpu_freq;
            float bytes_per_second = stat_result.st_size / seconds_elapsed;
            float gigabytes_per_second = bytes_per_second / 1000000000.0f;
            printf("cyles: %llu  (%f GB/second)\n", minimum_runtime, gigabytes_per_second);
            os_elapsed = 0;
            os_start_time = ReadOSTimer();
        }
        else
        {
            os_elapsed = ReadOSTimer() - os_start_time;
        }
    }

    buffer->data[stat_result.st_size] = 0; // null terminate
    buffer->i = 0;
    fclose(file);
}

typedef struct
{
    u64 Size;
    u8 *Data;
} buffer;

static void TestWritingToBuffer(buffer *Buffer)
{
    for (u64 I = 0; I < Buffer->Size; I++)
    {
        Buffer->Data[I] = I;
    }
}

#define BUFFER_SIZE 20000

typedef enum
{
    timed_block_Main,
} timed_block;

int main(void)
{
    buffer *Buffer = malloc(sizeof(buffer) + BUFFER_SIZE);
    Buffer->Size = BUFFER_SIZE;
    Buffer->Data = (u8 *)(Buffer + 1);

    BeginProfile();
    BEGIN_TIMED_BLOCK(timed_block_Main);

    TestWritingToBuffer(Buffer);

    END_TIMED_BLOCK(timed_block_Main);
    EndAndPrintProfile();
    printf("buffer size %llu\n", Buffer->Size);

    /* test_read_file("./dist/haversine.json"); */
    return 0;
}
