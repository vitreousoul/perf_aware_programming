global TestLoop

section .text

TestLoop:
    xor rax, rax
loop:
    mov [rsi + rax], al
    add rax, 1
    cmp rdi, rax
    jne loop
    ret
