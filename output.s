.intel_syntax noprefix
.text

.globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 48
    xor eax, eax

    lea rax, [rbp-4]
    mov QWORD PTR [rbp-12], rax
    mov rax, QWORD PTR [rbp-12]
    mov ecx, 10
    mov DWORD PTR [rax], ecx
    lea rax, [rbp-4]
    mov QWORD PTR [rbp-20], rax
    mov rax, QWORD PTR [rbp-20]
    mov ecx, 5
    mov DWORD PTR [rax], ecx
    lea rax, [rbp-4]
    mov QWORD PTR [rbp-28], rax
    mov rax, QWORD PTR [rbp-28]
    mov eax, DWORD PTR [rax]
    mov DWORD PTR [rbp-36], eax
    mov eax, DWORD PTR [rbp-36]
    jmp .L_main_exit
.L_main_exit:
    mov rsp, rbp
    pop rbp
    ret

.section .note.GNU-stack,"",@progbits
