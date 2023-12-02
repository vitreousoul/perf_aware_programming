global TestLoop

section .text

TestLoop:
    xor rax, rax
loop:
    add rax, 1
    cmp rdi, rax
    jne loop
    ret
