#include <stdlib.h>
#include <stdio.h>

#include "ryn_prof.h"

typedef enum
{
    TB_main,
} timed_block_kind;

typedef struct
{
    size_t Size;
    char *Data;
} buffer;

#define BUFFER_SIZE 4096

char Data[BUFFER_SIZE] = {};

void TestLoop(buffer Buffer);

int main(void)
{
    buffer Buffer;
    Buffer.Size = BUFFER_SIZE;
    Buffer.Data = Data;

    BeginProfile();

    BEGIN_BANDWIDTH_BLOCK(TB_main, BUFFER_SIZE);
    TestLoop(Buffer);
    END_TIMED_BLOCK(TB_main);

    EndAndPrintProfile();

    /* printf("%zu\n", Buffer.Size); */

#if 0
    for (int I = 0; I < Buffer.Size; ++I)
    {
        printf("%d ", Buffer.Data[I]);
    }
    printf("\n");
#endif
}
