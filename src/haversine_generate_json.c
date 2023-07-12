#include "haversine_generate_json.h"

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
    float offset = 0.0f, range = 0.0f;
    srand(seed);
    fprintf(file, "{\n    \"pairs\": [\n");
    for (i = 0; i < pairs_count; ++i)
    {
        float v[4];
        if (i % (pairs_count / 4) == 0)
        {
            offset = random_float_between_zero_and(180.0f);
            range = random_float_between_zero_and(40.0f);
        }
        fprintf(file, "        {");
        for (j = 0; j < 2; ++j)
        {
            float x = offset + random_float_between_zero_and(range);
            float y = offset + random_float_between_zero_and(range);
            if (x > 360.0f) x -= 360.0f;
            if (y > 360.0f) y -= 360.0f;
            v[j] = x;
            v[j+1] = y;
            fprintf(file, "\"%s\":%f%s ", value_key[j], x, maybe_comma(j, 3));
            fprintf(file, "\"%s\":%f%s ", value_key[j+1], y, maybe_comma(j, 3));
        }
        fprintf(file, "}%s\n", maybe_comma(i, pairs_count - 1));
        float haversine = haversine_of_degrees(v[0], v[1], v[2], v[3], EARTH_RADIUS_KM);
        average += haversine / (float)pairs_count;
    }
    fprintf(file, "    ],\n    \"average\":%f\n}\n", average);
    fclose(file);
    return average;
}
