.intel_syntax noprefix
.text

.globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 80
    xor eax, eax

    lea rax, [rbp-4]
    mov QWORD PTR [rbp-24], rax
    mov rax, QWORD PTR [rbp-24]
    mov ecx, 10
    mov DWORD PTR [rax], ecx
    lea rax, [rbp-4]
    add rax, 4
    mov QWORD PTR [rbp-32], rax
    mov rax, QWORD PTR [rbp-32]
    mov ecx, 20
    mov DWORD PTR [rax], ecx
    lea rax, [rbp-4]
    mov QWORD PTR [rbp-40], rax
    mov rax, QWORD PTR [rbp-40]
    mov eax, DWORD PTR [rax]
    mov DWORD PTR [rbp-48], eax
    lea rax, [rbp-4]
    add rax, 4
    mov QWORD PTR [rbp-56], rax
    mov rax, QWORD PTR [rbp-56]
    mov eax, DWORD PTR [rax]
    mov DWORD PTR [rbp-64], eax
    mov eax, DWORD PTR [rbp-48]
    mov ecx, DWORD PTR [rbp-64]
    add eax, ecx
    mov DWORD PTR [rbp-72], eax
    mov eax, DWORD PTR [rbp-72]
    jmp .L_main_exit
.L_main_exit:
    mov rsp, rbp
    pop rbp
    ret

.section .note.GNU-stack,"",@progbits
