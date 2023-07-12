#include "haversine.h"

float EARTH_RADIUS_KM = 6371.0f;

static float radians(float degrees)
{
    return 0.01745329f * degrees;
}

static float haversine_of_degrees(float x0, float y0, float x1, float y1, float r)
{
    float dy = radians(y1 - y0);
    float dx = radians(x1 - x0);
    y0 = radians(y0);
    y1 = radians(y1);
    float sign_dy_over_2 = sin(dy/2.0f);
    float sign_dx_over_2 = sin(dx/2.0f);
    float root_term = (sign_dy_over_2*sign_dy_over_2) + cos(y0)*cos(y1)*(sign_dx_over_2*sign_dx_over_2);
    float result = 2.0f*r*asin(sqrt(root_term));
    return result;
}

static float random_float_between_zero_and(float upper_bound)
{
    return upper_bound * ((float)rand()/(float)RAND_MAX);
}

static char *maybe_comma(int value, int no_comma_value)
{
    return (value == no_comma_value) ? "" : ",";
}

static float write_haversine_json(char *file_path, int seed, int pairs_count)
{
    int i, j;
    char *value_key[] = {"x0", "y0", "x1", "y1"};
    FILE *file = fopen(file_path, "wb");
    float average = 0.0f;
    srand(seed);
    fprintf(file, "{\n    \"pairs\": [\n");
    for (i = 0; i < pairs_count; ++i)
    {
        float v[4];
        fprintf(file, "        {");
        for (j = 0; j < 4; ++j)
        {
            float random_value = random_float_between_zero_and(360.0f);
            fprintf(file, "\"%s\":%f%s ", value_key[j], random_value, maybe_comma(j, 3));
            v[j] = random_value;
        }
        fprintf(file, "}%s\n", maybe_comma(i, pairs_count - 1));
        float haversine = haversine_of_degrees(v[0], v[1], v[2], v[3], EARTH_RADIUS_KM);
        average += haversine / (float)pairs_count;
    }
    fprintf(file, "    ],\n    \"average\":%f\n}\n", average);
    fclose(file);
    return average;
}

int main(int arg_count, char **args)
{
    if (arg_count < 3) printf("Usage:\n    ./test.sh [random_seed] [number_of_pairs]\n");
    int seed = atoi(args[1]);
    int pairs_count = atoi(args[2]);
    char *file_path = "../dist/haversine.json";
    float average = write_haversine_json(file_path, seed, pairs_count);
    printf("average %f", average);
    return 0;
}
