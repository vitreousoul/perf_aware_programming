#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_SPACE(c) ((c) == ' ' || (c) == '\r' || (c) == '\t' || (c) == '\n')

#define ARRAY_CHUNK_COUNT 1024

typedef enum
{
    Json_Value_Kind_NONE,
    Json_Value_Kind_String,
    Json_Value_Kind_Integer,
    Json_Value_Kind_Float,
    Json_Value_Kind_Object,
    Json_Value_Kind_Array,
} Json_Value_Kind;

typedef struct Json_Value Json_Value;
typedef struct Json_Array Json_Array;

struct Json_Array
{
    Json_Value *array[ARRAY_CHUNK_COUNT];
    int used;
    Json_Array *next;
};

typedef struct
{
    char *key[ARRAY_CHUNK_COUNT];
    Json_Value *value[ARRAY_CHUNK_COUNT];
    int used;
    struct Json_Object *next;
} Json_Object;

struct Json_Value
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
};

typedef struct
{
    char *data;
    size_t i;
} Json_Buffer;



Json_Value *parse_json(char *file_path);
static Json_Value *parse_json_value(Json_Buffer *buffer);

static Json_Array *create_json_array()
{
    Json_Array *result = malloc(sizeof(Json_Array));
    memset(result, 0, sizeof(Json_Array));
    return result;
}

static void json_array_push(Json_Array *array, Json_Value *value)
{
    printf("json_array_push %p %p\n", array, value);
    if (!array) array = create_json_array();
    while(1)
    {
        if (array->used >= ARRAY_CHUNK_COUNT)
        {
            if (!array->next)
            {
                array->next = create_json_array();
            }
            array = array->next;
        }
        else
        {
            printf("setting value\n");
            array->array[array->used] = value;
            array->used += 1;
            break;
        }
    }
}

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

static int min(int a, int b)
{
    return a < b ? a : b;
}

static void chomp_space(Json_Buffer *buffer)
{
    while(1)
    {
        char character = buffer->data[buffer->i];
        if (character == 0 || !IS_SPACE(character))
        {
            break;
        }
        buffer->i += 1;
    }
}

#define MAX_DIGIT_CHARACTERS 512
static Json_Value *parse_json_digit(Json_Buffer *buffer)
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

static Json_Value *parse_json_string(Json_Buffer *buffer)
{
    printf("parse_json_string\n");
    Json_Value *result = malloc(sizeof(Json_Value));
    buffer->i += 1; // we are on a quote character, so skip it
    size_t start = buffer->i;
    while(buffer->data[buffer->i])
    {
        char character = buffer->data[buffer->i];
        if(character == '\\')
        {
            buffer->i += 1;
        }
        else if (character == '"')
        {
            break;
        }
        buffer->i += 1;
    }
    size_t string_length = buffer->i - start;
    buffer->i += 1; // skip over closing quote
    result->kind = Json_Value_Kind_String;
    result->string = malloc(string_length + 1);
    memcpy(result->string, &buffer->data[start], string_length);
    result->string[string_length] = 0;
    return result;
}

static Json_Value *parse_json_array(Json_Buffer *buffer)
{
    Json_Value *result = malloc(sizeof(Json_Value));
    result->kind = Json_Value_Kind_Array;
    buffer->i += 1; // skip over the open bracket
    while(1)
    {
        chomp_space(buffer);
        printf("ready to parse item for array\n");
        Json_Value *item = parse_json_value(buffer);
        if (!item)
        {
            printf("parse_json_array null item\n");
            return 0;
        }
        json_array_push(result->array, item);
        chomp_space(buffer);
        printf("%c\n", buffer->data[buffer->i]);
        if (buffer->data[buffer->i] == ',')
        {
            buffer->i += 1;
            continue;
        }
        else if (buffer->data[buffer->i] == ']')
        {
            break;
        }
        else
        {
            printf("parse_json_array expected comma or close-bracket, but got '%c'\n", buffer->data[buffer->i]);
            return 0;
        }
    }
    return result;
}

static Json_Value *parse_json_value(Json_Buffer *buffer)
{
    printf("parse_json_value\n");
    Json_Value *result = 0;
    chomp_space(buffer);
    switch(buffer->data[buffer->i])
    {
    case '"':
        result = parse_json_string(buffer);
        break;
    case '{':
        printf("parse_json object not implemented\n");
        return 0;
        break;
    case '[':
        result = parse_json_array(buffer);
        break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        result = parse_json_digit(buffer);
        break;
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
