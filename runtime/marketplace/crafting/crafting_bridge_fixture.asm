; Synthetic test functions only. Never link into the ASI.
option casemap:none
EXTERN crimson_crafting_test_avx:DWORD
PUBLIC crimson_crafting_fixture_probe
PUBLIC crimson_crafting_fixture_resume
PUBLIC crimson_crafting_fixture_alternate
PUBLIC crimson_crafting_fixture_clobber
PUBLIC crimson_crafting_fixture_bad_alignment
PUBLIC crimson_crafting_fixture_stable_pointer
.data
ALIGN 8
crimson_crafting_fixture_bad_alignment DWORD 0
crimson_crafting_fixture_stable_pointer DWORD 1
ALIGN 16
seed_pattern LABEL BYTE
    BYTE 37,54,71,88,105,122,139,156,173,190,207,224,241,2,19,36
    BYTE 53,70,87,104,121,138,155,172,189,206,223,240,1,18,35,52
    BYTE 69,86,103,120,137,154,171,188,205,222,239,0,17,34,51,68
    BYTE 85,102,119,136,153,170,187,204,221,238,255,16,33,50,67,84
    BYTE 101,118,135,152,169,186,203,220,237,254,15,32,49,66,83,100
    BYTE 117,134,151,168,185,202,219,236,253,14,31,48,65,82,99,116
    BYTE 133,150,167,184,201,218,235,252,13,30,47,64,81,98,115,132
    BYTE 149,166,183,200,217,234,251,12,29,46,63,80,97,114,131,148
    BYTE 165,182,199,216,233,250,11,28,45,62,79,96,113,130,147,164
    BYTE 181,198,215,232,249,10,27,44,61,78,95,112,129,146,163,180
    BYTE 197,214,231,248,9,26,43,60,77,94,111,128,145,162,179,196
    BYTE 213,230,247,8,25,42,59,76,93,110,127,144,161,178,195,212
    BYTE 229,246,7,24,41,58,75,92,109,126,143,160,177,194,211,228
    BYTE 245,6,23,40,57,74,91,108,125,142,159,176,193,210,227,244
    BYTE 5,22,39,56,73,90,107,124,141,158,175,192,209,226,243,4
    BYTE 21,38,55,72,89,106,123,140,157,174,191,208,225,242,3,20
    BYTE 37,54,71,88,105,122,139,156,173,190,207,224,241,2,19,36
    BYTE 53,70,87,104,121,138,155,172,189,206,223,240,1,18,35,52
    BYTE 69,86,103,120,137,154,171,188,205,222,239,0,17,34,51,68
    BYTE 85,102,119,136,153,170,187,204,221,238,255,16,33,50,67,84
    BYTE 101,118,135,152,169,186,203,220,237,254,15,32,49,66,83,100
    BYTE 117,134,151,168,185,202,219,236,253,14,31,48,65,82,99,116
    BYTE 133,150,167,184,201,218,235,252,13,30,47,64,81,98,115,132
    BYTE 149,166,183,200,217,234,251,12,29,46,63,80,97,114,131,148
    BYTE 165,182,199,216,233,250,11,28,45,62,79,96,113,130,147,164
    BYTE 181,198,215,232,249,10,27,44,61,78,95,112,129,146,163,180
    BYTE 197,214,231,248,9,26,43,60,77,94,111,128,145,162,179,196
    BYTE 213,230,247,8,25,42,59,76,93,110,127,144,161,178,195,212
    BYTE 229,246,7,24,41,58,75,92,109,126,143,160,177,194,211,228
    BYTE 245,6,23,40,57,74,91,108,125,142,159,176,193,210,227,244
    BYTE 5,22,39,56,73,90,107,124,141,158,175,192,209,226,243,4
    BYTE 21,38,55,72,89,106,123,140,157,174,191,208,225,242,3,20
seed_mxcsr DWORD 05f80h
normal_mxcsr DWORD 01f80h
.code
crimson_crafting_fixture_probe PROC
    pushfq
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 192
    mov [rsp+160], rcx
    mov [rsp+168], rdx
    mov [rcx+2688], rsp
    fxsave64 [rcx]
    fninit
    fld1
    fldpi
    ldmxcsr seed_mxcsr
    ; The direct no-bridge control reproduces occasional OS truncation of the
    ; high x87 instruction-pointer bits. Normal fixtures use canonical low
    ; diagnostic FIP/FDP values to isolate the bridge from that external effect.
    cmp crimson_crafting_fixture_stable_pointer, 0
    je pointer_seeded
    fxsave64 [rcx+512]
    mov qword ptr [rcx+520], 12345678h
    mov qword ptr [rcx+528], 23456789h
    fxrstor64 [rcx+512]
pointer_seeded:
    cmp crimson_crafting_test_avx, 0
    je seed_legacy
    vmovdqu ymm0, ymmword ptr [seed_pattern+0]
    vmovdqu ymm1, ymmword ptr [seed_pattern+32]
    vmovdqu ymm2, ymmword ptr [seed_pattern+64]
    vmovdqu ymm3, ymmword ptr [seed_pattern+96]
    vmovdqu ymm4, ymmword ptr [seed_pattern+128]
    vmovdqu ymm5, ymmword ptr [seed_pattern+160]
    vmovdqu ymm6, ymmword ptr [seed_pattern+192]
    vmovdqu ymm7, ymmword ptr [seed_pattern+224]
    vmovdqu ymm8, ymmword ptr [seed_pattern+256]
    vmovdqu ymm9, ymmword ptr [seed_pattern+288]
    vmovdqu ymm10, ymmword ptr [seed_pattern+320]
    vmovdqu ymm11, ymmword ptr [seed_pattern+352]
    vmovdqu ymm12, ymmword ptr [seed_pattern+384]
    vmovdqu ymm13, ymmword ptr [seed_pattern+416]
    vmovdqu ymm14, ymmword ptr [seed_pattern+448]
    vmovdqu ymm15, ymmword ptr [seed_pattern+480]
    fxsave64 [rcx+512]
    vmovdqu ymmword ptr [rcx+1536+0], ymm0
    vmovdqu ymmword ptr [rcx+1536+32], ymm1
    vmovdqu ymmword ptr [rcx+1536+64], ymm2
    vmovdqu ymmword ptr [rcx+1536+96], ymm3
    vmovdqu ymmword ptr [rcx+1536+128], ymm4
    vmovdqu ymmword ptr [rcx+1536+160], ymm5
    vmovdqu ymmword ptr [rcx+1536+192], ymm6
    vmovdqu ymmword ptr [rcx+1536+224], ymm7
    vmovdqu ymmword ptr [rcx+1536+256], ymm8
    vmovdqu ymmword ptr [rcx+1536+288], ymm9
    vmovdqu ymmword ptr [rcx+1536+320], ymm10
    vmovdqu ymmword ptr [rcx+1536+352], ymm11
    vmovdqu ymmword ptr [rcx+1536+384], ymm12
    vmovdqu ymmword ptr [rcx+1536+416], ymm13
    vmovdqu ymmword ptr [rcx+1536+448], ymm14
    vmovdqu ymmword ptr [rcx+1536+480], ymm15
    jmp seed_registers
seed_legacy:
    movdqu xmm0, xmmword ptr [seed_pattern+0]
    movdqu xmm1, xmmword ptr [seed_pattern+32]
    movdqu xmm2, xmmword ptr [seed_pattern+64]
    movdqu xmm3, xmmword ptr [seed_pattern+96]
    movdqu xmm4, xmmword ptr [seed_pattern+128]
    movdqu xmm5, xmmword ptr [seed_pattern+160]
    movdqu xmm6, xmmword ptr [seed_pattern+192]
    movdqu xmm7, xmmword ptr [seed_pattern+224]
    movdqu xmm8, xmmword ptr [seed_pattern+256]
    movdqu xmm9, xmmword ptr [seed_pattern+288]
    movdqu xmm10, xmmword ptr [seed_pattern+320]
    movdqu xmm11, xmmword ptr [seed_pattern+352]
    movdqu xmm12, xmmword ptr [seed_pattern+384]
    movdqu xmm13, xmmword ptr [seed_pattern+416]
    movdqu xmm14, xmmword ptr [seed_pattern+448]
    movdqu xmm15, xmmword ptr [seed_pattern+480]
    fxsave64 [rcx+512]
seed_registers:
    mov rax, 01111111111111111h
    mov rcx, 02222222222222222h
    mov rdx, 03333333333333333h
    mov rbx, 04444444444444444h
    mov rbp, 05555555555555555h
    mov rsi, 06666666666666666h
    mov rdi, 07777777777777777h
    mov r8, 08888888888888888h
    mov r9, 09999999999999999h
    mov r10, 0aaaaaaaaaaaaaaaah
    mov r11, 0bbbbbbbbbbbbbbbbh
    mov r12, 0cccccccccccccccch
    mov r13, 0ddddddddddddddddh
    mov r14, 0eeeeeeeeeeeeeeeeh
    mov r15, 0ffffffffffffffffh
    push 0cd7h
    popfq
    jmp qword ptr [rsp+168]
crimson_crafting_fixture_probe ENDP
crimson_crafting_fixture_alternate PROC
    jmp crimson_crafting_fixture_resume
crimson_crafting_fixture_alternate ENDP
crimson_crafting_fixture_resume PROC
    pushfq
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rbx, rsp
    cld
    mov rdi, [rbx+128+160]
    fxsave64 [rdi+1024]
    cmp crimson_crafting_test_avx, 0
    je after_vector_saved
    vmovdqu ymmword ptr [rdi+2048+0], ymm0
    vmovdqu ymmword ptr [rdi+2048+32], ymm1
    vmovdqu ymmword ptr [rdi+2048+64], ymm2
    vmovdqu ymmword ptr [rdi+2048+96], ymm3
    vmovdqu ymmword ptr [rdi+2048+128], ymm4
    vmovdqu ymmword ptr [rdi+2048+160], ymm5
    vmovdqu ymmword ptr [rdi+2048+192], ymm6
    vmovdqu ymmword ptr [rdi+2048+224], ymm7
    vmovdqu ymmword ptr [rdi+2048+256], ymm8
    vmovdqu ymmword ptr [rdi+2048+288], ymm9
    vmovdqu ymmword ptr [rdi+2048+320], ymm10
    vmovdqu ymmword ptr [rdi+2048+352], ymm11
    vmovdqu ymmword ptr [rdi+2048+384], ymm12
    vmovdqu ymmword ptr [rdi+2048+416], ymm13
    vmovdqu ymmword ptr [rdi+2048+448], ymm14
    vmovdqu ymmword ptr [rdi+2048+480], ymm15
after_vector_saved:
    lea rax, [rbx+128]
    mov [rdi+2696], rax
    mov rsi, rbx
    lea rdi, [rdi+2560]
    mov ecx, 16
    rep movsq
    mov rdi, [rbx+128+160]
    fxrstor64 [rdi]
    lea rsp, [rbx+128+192]
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    popfq
    ret
crimson_crafting_fixture_resume ENDP
crimson_crafting_fixture_clobber PROC
    lea rax, [rsp+8]
    and eax, 15
    jz correctly_aligned
    inc crimson_crafting_fixture_bad_alignment
correctly_aligned:
    mov qword ptr [rsp+8], 11h
    mov qword ptr [rsp+16], 22h
    mov qword ptr [rsp+24], 33h
    mov qword ptr [rsp+32], 44h
    sub rsp, 160
    movdqu xmmword ptr [rsp+0], xmm6
    movdqu xmmword ptr [rsp+16], xmm7
    movdqu xmmword ptr [rsp+32], xmm8
    movdqu xmmword ptr [rsp+48], xmm9
    movdqu xmmword ptr [rsp+64], xmm10
    movdqu xmmword ptr [rsp+80], xmm11
    movdqu xmmword ptr [rsp+96], xmm12
    movdqu xmmword ptr [rsp+112], xmm13
    movdqu xmmword ptr [rsp+128], xmm14
    movdqu xmmword ptr [rsp+144], xmm15
    fninit
    ldmxcsr normal_mxcsr
    cmp crimson_crafting_test_avx, 0
    je clobber_legacy
    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3
    vpxor ymm4, ymm4, ymm4
    vpxor ymm5, ymm5, ymm5
    vpxor ymm6, ymm6, ymm6
    vpxor ymm7, ymm7, ymm7
    vpxor ymm8, ymm8, ymm8
    vpxor ymm9, ymm9, ymm9
    vpxor ymm10, ymm10, ymm10
    vpxor ymm11, ymm11, ymm11
    vpxor ymm12, ymm12, ymm12
    vpxor ymm13, ymm13, ymm13
    vpxor ymm14, ymm14, ymm14
    vpxor ymm15, ymm15, ymm15
    vmovdqu xmm6, xmmword ptr [rsp+0]
    vmovdqu xmm7, xmmword ptr [rsp+16]
    vmovdqu xmm8, xmmword ptr [rsp+32]
    vmovdqu xmm9, xmmword ptr [rsp+48]
    vmovdqu xmm10, xmmword ptr [rsp+64]
    vmovdqu xmm11, xmmword ptr [rsp+80]
    vmovdqu xmm12, xmmword ptr [rsp+96]
    vmovdqu xmm13, xmmword ptr [rsp+112]
    vmovdqu xmm14, xmmword ptr [rsp+128]
    vmovdqu xmm15, xmmword ptr [rsp+144]
    jmp clobber_registers
clobber_legacy:
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    pxor xmm4, xmm4
    pxor xmm5, xmm5
clobber_registers:
    add rsp, 160
    mov rax, 0123456789abcdef0h
    mov rcx, 0123456789abcdef0h
    mov rdx, 0123456789abcdef0h
    mov r8, 0123456789abcdef0h
    mov r9, 0123456789abcdef0h
    mov r10, 0123456789abcdef0h
    mov r11, 0123456789abcdef0h
    xor eax, eax
    ret
crimson_crafting_fixture_clobber ENDP
END
