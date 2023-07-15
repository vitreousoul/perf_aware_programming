#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_SPACE(c) ((c) == ' ' || (c) == '\r' || (c) == '\t' || (c) == '\n')

#define ARRAY_CHUNK_COUNT 2048

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
typedef struct Json_Object Json_Object;

struct Json_Array
{
    Json_Value *value[ARRAY_CHUNK_COUNT];
    int used;
    Json_Array *next;
};

struct Json_Object
{
    char *key[ARRAY_CHUNK_COUNT];
    Json_Value *value[ARRAY_CHUNK_COUNT];
    int used;
    Json_Object *next;
};

struct Json_Value
{
    Json_Value_Kind kind;
    union
    {
        char *string;
        float number_float;
        int number_integer;
        Json_Array *array;
        Json_Object *object;
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

static Json_Object *create_json_object()
{
    Json_Object *result = malloc(sizeof(Json_Object));
    // NOTE: the key/value arrays are unitialized, but that's ok because we overwrite them before reading the values.
    result->next = 0;
    result->used = 0;
    return result;
}

static void json_array_push(Json_Array *array, Json_Value *value)
{
    BEGIN_TIMED_BLOCK(Timer_json_array_push);
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
            array->value[array->used] = value;
            array->used += 1;
            break;
        }
    }
    END_TIMED_BLOCK(Timer_json_array_push);
}

static void json_object_push(Json_Object *object, char *key, Json_Value *value)
{
    BEGIN_TIMED_BLOCK(Timer_json_object_push);
    if (!object) object = create_json_object();
    while(1)
    {
        if (object->used >= ARRAY_CHUNK_COUNT)
        {
            if (!object->next)
            {
                object->next = create_json_object();
            }
            object = object->next;
        }
        else
        {
            object->key[object->used] = key;
            object->value[object->used] = value;
            object->used += 1;
            break;
        }
    }
    END_TIMED_BLOCK(Timer_json_object_push);
}

static Json_Buffer *read_file(char *file_path)
{
    BEGIN_TIMED_BLOCK(Timer_parse_read);
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
    END_TIMED_BLOCK(Timer_parse_read);
    return buffer;
}

static int min(int a, int b)
{
    return a < b ? a : b;
}

static void chomp_space(Json_Buffer *buffer)
{
    /* BEGIN_TIMED_BLOCK(Timer_chomp_space); */
    while(1)
    {
        char character = buffer->data[buffer->i];
        if (character == 0 || !IS_SPACE(character))
        {
            break;
        }
        buffer->i += 1;
    }
    /* END_TIMED_BLOCK(Timer_chomp_space); */
}

#define MAX_DIGIT_CHARACTERS 512
char digit_characters[MAX_DIGIT_CHARACTERS] = {};
static Json_Value *parse_json_digit(Json_Buffer *buffer)
{
    BEGIN_TIMED_BLOCK(Timer_parse_json_digit);
    int start = buffer->i, is_float = 0;
    Json_Value *result = malloc(sizeof(Json_Value));
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
    digit_characters[digit_length] = 0;
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
    END_TIMED_BLOCK(Timer_parse_json_digit);
    return result;
}

static Json_Value *parse_json_string(Json_Buffer *buffer)
{
    BEGIN_TIMED_BLOCK(Timer_parse_json_string);
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
    END_TIMED_BLOCK(Timer_parse_json_string);
    return result;
}

static Json_Value *parse_json_array(Json_Buffer *buffer)
{
    BEGIN_TIMED_BLOCK(Timer_parse_json_array);

    Json_Value *result = malloc(sizeof(Json_Value));
    result->kind = Json_Value_Kind_Array;
    result->array = create_json_array();
    buffer->i += 1; // skip over the open bracket
    while(1)
    {
        chomp_space(buffer);
        Json_Value *item = parse_json_value(buffer);
        if (!item)
        {
            printf("parse_json_array null item\n");
            return 0;
        }
        json_array_push(result->array, item);
        chomp_space(buffer);
        if (buffer->data[buffer->i] == ',')
        {
            buffer->i += 1;
            continue;
        }
        else if (buffer->data[buffer->i] == ']')
        {
            buffer->i += 1;
            break;
        }
        else
        {
            printf("parse_json_array expected comma or close-bracket, but got '%c'\n", buffer->data[buffer->i]);
            return 0;
        }
    }
    END_TIMED_BLOCK(Timer_parse_json_array);
    return result;
}

static Json_Value *parse_json_object(Json_Buffer *buffer)
{
    BEGIN_TIMED_BLOCK(Timer_parse_json_object);
    Json_Value *result = malloc(sizeof(Json_Value));
    result->kind = Json_Value_Kind_Object;
    result->object = create_json_object();
    buffer->i += 1; // skip over the open bracket
    for(;;)
    {
        chomp_space(buffer);
        Json_Value *key = parse_json_value(buffer);
        chomp_space(buffer);
        if (buffer->data[buffer->i] != ':')
        {
            printf("parse_json_object expected ':' but got %c\n", buffer->data[buffer->i]);
        }
        buffer->i += 1; // skip over colon
        chomp_space(buffer);
        Json_Value *value = parse_json_value(buffer);
        json_object_push(result->object, key->string, value);
        chomp_space(buffer);
        if (buffer->data[buffer->i] == ',')
        {
            buffer->i += 1;
            continue;
        }
        else if (buffer->data[buffer->i] == '}')
        {
            buffer->i += 1;
            break;
        }
        else
        {
            printf("parse_json_object expected comma or close-bracket, but got '%c'\n", buffer->data[buffer->i]);
            return 0;
        }
    }
    END_TIMED_BLOCK(Timer_parse_json_object);
    return result;
}

static Json_Value *parse_json_value(Json_Buffer *buffer)
{
    BEGIN_TIMED_BLOCK(Timer_parse_json_value);
    Json_Value *result = 0;
    chomp_space(buffer);
    switch(buffer->data[buffer->i])
    {
    case '"':
        result = parse_json_string(buffer);
        break;
    case '{':
        result = parse_json_object(buffer);
        break;
    case '[':
        result = parse_json_array(buffer);
        break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
        result = parse_json_digit(buffer);
    } break;
    default:
        printf("Error parsing json value, found '%c'\n", buffer->data[buffer->i]);
        return 0;
    }
    END_TIMED_BLOCK(Timer_parse_json_value);
    return result;
}

static void print_json(Json_Value *value)
{
    if (!value) return;
    switch(value->kind)
    {
    case Json_Value_Kind_String: printf("\"%s\"", value->string); break;
    case Json_Value_Kind_Integer: printf("%d", value->number_integer); break;
    case Json_Value_Kind_Float: printf("%f", value->number_float); break;
    case Json_Value_Kind_Object:
    {
        printf("{");
        Json_Object *object = value->object;
        while(object)
        {
            int i;
            for (i = 0; i < object->used; i++)
            {
                printf("\"%s\":", object->key[i]);
                print_json(object->value[i]);
                if (i < object->used - 1) printf(",");
            }
            object = object->next;
        }
        printf("}");
    } break;
    case Json_Value_Kind_Array:
    {
        printf("[");
        Json_Array *array = value->array;
        while(array)
        {
            int i;
            for (i = 0; i < array->used; i++)
            {
                print_json(array->value[i]);
                if (i < array->used - 1) printf(",");
            }
            array = array->next;
        }
        printf("]");
    } break;
    default: return;
    }
}

Json_Value *parse_json(char *file_path)
{
    BEGIN_TIMED_BLOCK(Timer_parse);
    Json_Buffer *buffer = read_file(file_path);
    Json_Value *result = parse_json_value(buffer);
    if (0) print_json(result);
    /* printf("\n"); */
    END_TIMED_BLOCK(Timer_parse);
    return result;
}
