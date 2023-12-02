#include <stdio.h>
#include <stdlib.h>

typedef uint64_t u64;

#include "ryn_prof.h"

/* JumpIf0 and JumpIf1 are defined in branch_predictor_test.asm */
int JumpIf0(int Value);
int JumpIf1(int Value);

typedef struct
{
    u64 Size;
    char *Data;
} buffer;

typedef enum
{
    TB_main,
} timed_block;


#define BufferSize 256
static u64 Results[BufferSize] = {0};


void TestBranchPredictor1(buffer Buffer)
{
    BeginProfile();

    for (int I = 0; I < Buffer.Size; ++I)
    {
        Results[I] = JumpIf1(Buffer.Data[I]);
    }

    EndProfile();
}

void TestBranchPredictor0(buffer Buffer)
{
    BeginProfile();

    for (int I = 0; I < Buffer.Size; ++I)
    {
        Results[I] = JumpIf0(Buffer.Data[I]);
    }

    EndProfile();
}

int Zero(int I) {return 0;}
int One(int I) {return 1;}
int IMod2(int I) {return I % 2;}
int IPlus1Mod2(int I) {return (I + 1) % 2;}
int FirstHalfZerosSecondHalfOnes(int I) {return (I < BufferSize / 2) ? 0 : 1;}
int FirstHalfOnesSecondHalfZeros(int I) {return (I < BufferSize / 2) ? 1 : 0;}

#define PRINT_BUFFER 0

void InitBuffer(int (*BufferInitFunction)(int), buffer Buffer)
{
    for (int I = 0; I < Buffer.Size; ++I)
    {
        Buffer.Data[I] = BufferInitFunction(I);
#if PRINT_BUFFER
        if (I % 16 == 0) printf("\n");
        printf("%d ", Buffer.Data[I]);
#endif
    }
#if PRINT_BUFFER
    printf("\n");
#endif
}

int TestBranch(void (*TestFunction)(buffer), int (*BufferInitFunction)(int), buffer Buffer)
{
    u64 MinTime = 0xffffffffffffffff;
    InitBuffer(BufferInitFunction, Buffer);

    for (int I = 0; I < 4000000; ++I)
    {
        TestFunction(Buffer);

        /* GlobalProfiler is defined in ryn_prof.h */
        u64 TimeTaken = GlobalProfiler.EndTime - GlobalProfiler.StartTime;

        if (TimeTaken < MinTime)
        {
            MinTime = TimeTaken;
            I = 0;
        }
    }

    return MinTime;
}

typedef struct
{
    int (*BufferInitFunction)(int);
    char *Name;
} test_item;

int main(void)
{
    char Data[BufferSize] = {0};
    buffer Buffer;
    Buffer.Size = BufferSize;
    Buffer.Data = Data;

    test_item TestItems[] = {
        {Zero, "0"},
        {One, "1"},
        {IMod2, "(I)%2"},
        {IPlus1Mod2, "(I+1)%2"},
        {FirstHalfZerosSecondHalfOnes, "[0..01..1]"},
        {FirstHalfOnesSecondHalfZeros, "[1..10..0]"}
    };

    for (int I = 0; I < ArrayCount(TestItems); ++I)
    {
        printf("%s,", TestItems[I].Name);
    }
    printf("\n");

    /* Test jump-if-0 */
    for (int TestCount = 0; TestCount < 4; ++TestCount)
    {
        for (int I = 0; I < ArrayCount(TestItems); ++I)
        {
            int Result = TestBranch(TestBranchPredictor0, TestItems[I].BufferInitFunction, Buffer);
            printf("%d,", Result);
        }
        printf("\n");
    }

    /* Test jump-if-1 */
    for (int TestCount = 0; TestCount < 4; ++TestCount)
    {
        for (int I = 0; I < ArrayCount(TestItems); ++I)
        {
            int Result = TestBranch(TestBranchPredictor1, TestItems[I].BufferInitFunction, Buffer);
            printf("%d,", Result);
        }
        printf("\n");
    }
    printf("\n");

    int NoWayThisIsTrue = Buffer.Data[24] == 0xf;
    /* Make the compiler think we use Results, to prevent it getting compiled away. */
    if (NoWayThisIsTrue)
    {
        for (int I = 0; I < BufferSize; ++I)
        {
            if (I % 16 == 0) printf("\n");
            printf("%llu ", Results[I]);
        }
        printf("\n");
    }

    return 0;
}
