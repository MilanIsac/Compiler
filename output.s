.intel_syntax noprefix
.text

.globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    xor eax, eax

    mov eax, 10
    mov DWORD PTR [rbp-4], eax
    mov eax, 20
    mov DWORD PTR [rbp-4], eax
    mov eax, DWORD PTR [rbp-4]
    jmp .L_main_exit

.L_main_exit:
    mov rsp, rbp
    pop rbp
    ret

.section .note.GNU-stack,"",@progbits
