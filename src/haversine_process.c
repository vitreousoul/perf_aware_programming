#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

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
    while(string_a[i] != 0 && string_b[i] != 0)
    {
        if (string_a[i] != string_b[i])
        {
            result = 0;
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

int main(int arg_count, char **args)
{
    if (arg_count < 2)
    {
        print_usage();
        return 1;
    }
    char *mode = args[1];
    if (string_match(mode, "process"))
    {
        printf("time to process!\n");
        parse_json("../dist/haversine.json");
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
        printf("average %f\n", average);
    }
    return 0;
}
