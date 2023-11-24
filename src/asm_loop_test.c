#include <stdlib.h>
#include <stdio.h>

typedef struct
{
    size_t Size;
    char *Data;
} buffer;

char Data[256] = {};

void TestLoop(buffer Buffer);

int main(void)
{
    buffer Buffer;
    Buffer.Size = 32;
    Buffer.Data = Data;

    TestLoop(Buffer);

    for (int I = 0; I < Buffer.Size; ++I)
    {
        printf("%d ", Buffer.Data[I]);
    }

    printf("\n");
}
