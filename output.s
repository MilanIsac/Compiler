.intel_syntax noprefix
.text

.globl cse
cse:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    xor eax, eax

    mov DWORD PTR [rbp-4], edi
    mov DWORD PTR [rbp-12], esi
    mov eax, DWORD PTR [rbp-4]
    mov ecx, DWORD PTR [rbp-12]
    add eax, ecx
    mov DWORD PTR [rbp-20], eax
    mov eax, DWORD PTR [rbp-20]
    mov ecx, DWORD PTR [rbp-20]
    add eax, ecx
    mov DWORD PTR [rbp-28], eax
    mov eax, DWORD PTR [rbp-28]
    jmp .L_cse_exit
.L_cse_exit:
    mov rsp, rbp
    pop rbp
    ret

.section .note.GNU-stack,"",@progbits
