typedef enum
{
    Json_Value_Kind_NONE,
    Json_Value_Kind_String,
    Json_Value_Kind_Integer,
    Json_Value_Kind_Float,
    Json_Value_Kind_Object,
    Json_Value_Kind_Array,
} Json_Value_Kind;

struct Json_Array;
struct Json_Object;

typedef struct
{
    Json_Value_Kind kind;
    union
    {
        char *string;
        float number_float;
        int number_integer;
        struct Json_Array *array;
        struct Json_Object *object;
    };
} Json_Value;

typedef struct
{
    Json_Value array[1024];
    int size;
    struct Json_Array *node;
} Json_Array;

typedef struct
{
    Json_Value key[1024];
    Json_Value value[1024];
    int size;
    struct Json_Object *node;
} Json_Object;

static char *read_file(char *file_path)
{
    struct stat stat_result;
    int stat_error = stat(file_path, &stat_result);
    if (stat_error)
    {
        printf("read_file stat error\n");
        return "";
    }
    char *data = malloc(stat_result.st_size);
    FILE *file = fopen(file_path, "rb");
    fread(data, 1, stat_result.st_size, file);
    fclose(file);
    return data;
}

Json_Value *parse_json(char *file_path);

Json_Value *parse_json(char *file_path)
{
    int i = 0, running = 1;
    Json_Value *result = 0;
    char *source = read_file(file_path);
    printf("\n");
    while(running)
    {
        switch(source[i])
        {
        case 0:
            running = 0;
            break;
        default:
            printf("%c", source[i]);
        }
        i++;
    }
    printf("\n");
    return result;
}
