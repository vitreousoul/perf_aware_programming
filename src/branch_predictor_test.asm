global JumpIf0
global JumpIf1

section .text

JumpIf0:
    cmp rdi, 0
    je Is0
    mov rax, 13
    ret
Is0:
    mov rax, 84
    ret
    %rep 14
    nop
    %endrep

JumpIf1:
    cmp rdi, 1
    je Is1
    mov rax, 13
    ret
Is1:
    mov rax, 84
    ret
