#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

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
    char *key[1024];
    Json_Value value[1024];
    int size;
    struct Json_Object *node;
} Json_Object;

typedef struct
{
    char *data;
    size_t i;
} Json_Buffer;

static Json_Buffer *read_file(char *file_path)
{
    struct stat stat_result;
    int stat_error = stat(file_path, &stat_result);
    if (stat_error)
    {
        printf("read_file stat error\n");
        return 0;
    }
    Json_Buffer *buffer = malloc(sizeof(Json_Buffer));
    FILE *file = fopen(file_path, "rb");
    buffer->data = malloc(stat_result.st_size + 1);
    fread(buffer->data, 1, stat_result.st_size, file);
    buffer->data[stat_result.st_size] = 0; // null terminate
    buffer->i = 0;
    fclose(file);
    return buffer;
}

Json_Value *parse_json(char *file_path);
Json_Value *parse_json_digit(Json_Buffer *buffer);

static int min(int a, int b)
{
    return a < b ? a : b;
}

#define MAX_DIGIT_CHARACTERS 512
Json_Value *parse_json_digit(Json_Buffer *buffer)
{
    int start = buffer->i, is_float = 0;
    Json_Value *result = malloc(sizeof(Json_Value));
    char digit_characters[MAX_DIGIT_CHARACTERS] = {};
    while(buffer->data[buffer->i])
    {
        char character = buffer->data[buffer->i];
        if(character == '.')
        {
            if(is_float)
            {
                printf("unexpected period parsing float\n");
                return 0;
            }
            is_float = 1;
        }
        else if(!IS_DIGIT(character))
        {
            break;
        }
        buffer->i++;
    }
    int digit_length = min(buffer->i - start, MAX_DIGIT_CHARACTERS - 2);
    memcpy(digit_characters, &buffer->data[start], digit_length);
    digit_characters[buffer->i + digit_length] = 0;
    if (is_float)
    {
        result->kind = Json_Value_Kind_Float;
        result->number_float = atof(digit_characters);
    }
    else
    {
        result->kind = Json_Value_Kind_Integer;
        result->number_integer = atoi(digit_characters);
    }
    return result;
}

static Json_Value *parse_json_value(Json_Buffer *buffer)
{
    Json_Value *result = 0;
    switch(buffer->data[buffer->i])
    {
    case '"':
        printf("parse_json string not implemented\n");
        return 0;
        break;
    case '{':
        printf("parse_json object not implemented\n");
        return 0;
        break;
    case '[':
        printf("parse_json array not implemented\n");
        return 0;
        break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
        result = parse_json_digit(buffer);
    } break;
    default:
        printf("Error parsing json value, found '%c'", buffer->data[buffer->i]);
        return 0;
    }
    return result;
}

Json_Value *parse_json(char *file_path)
{
    Json_Buffer *buffer = read_file(file_path);
    Json_Value *result = parse_json_value(buffer);
    return result;
}
