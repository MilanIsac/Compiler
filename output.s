.intel_syntax noprefix
.text

.globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    xor eax, eax

    mov edi, 10
    mov esi, 5
    call add
    mov DWORD PTR [rbp-4], eax
    mov eax, DWORD PTR [rbp-4]
    jmp .L_main_exit

.L_main_exit:
    mov rsp, rbp
    pop rbp
    ret

.globl add
add:
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
    jmp .L_add_exit
.L_add_exit:
    mov rsp, rbp
    pop rbp
    ret

.section .note.GNU-stack,"",@progbits
