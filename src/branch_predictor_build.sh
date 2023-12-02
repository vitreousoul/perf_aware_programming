#!/usr/bin/env sh

nasm --gprefix _ -f macho64 branch_predictor_test.asm -o branch_predictor_testasm.o
gcc -O1 branch_predictor_testasm.o branch_predictor_test.c
