option casemap:none
EXTERN crimson_crafting_dispatch:PROC
EXTERN crimson_crafting_state_bytes:DWORD
EXTERN crimson_crafting_state_mode:DWORD
EXTERN crimson_crafting_state_low:DWORD
EXTERN crimson_crafting_state_high:DWORD
PUBLIC crimson_crafting_database_entry
PUBLIC crimson_crafting_client_entry
PUBLIC crimson_crafting_quantity_entry
PUBLIC crimson_crafting_knowledge_a_entry
PUBLIC crimson_crafting_knowledge_b_entry
.code

crimson_crafting_database_entry PROC
    push 0
    jmp crimson_crafting_bridge
crimson_crafting_database_entry ENDP
crimson_crafting_client_entry PROC
    push 1
    jmp crimson_crafting_bridge
crimson_crafting_client_entry ENDP
crimson_crafting_quantity_entry PROC
    push 2
    jmp crimson_crafting_bridge
crimson_crafting_quantity_entry ENDP
crimson_crafting_knowledge_a_entry PROC
    push 3
    jmp crimson_crafting_bridge
crimson_crafting_knowledge_a_entry ENDP
crimson_crafting_knowledge_b_entry PROC
    push 4
    jmp crimson_crafting_bridge
crimson_crafting_knowledge_b_entry ENDP

; Internal detour boundary, entered by JMP, not a C++ function. Save every GPR
; and flags before using scratch registers. Never put a function call before the
; complete enabled processor state has been saved.
crimson_crafting_bridge PROC
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
    mov eax, crimson_crafting_state_bytes
    add rax, 128
    mov r10, rsp
    sub r10, rax
    and r10, -64
    ; Touch every intervening stack page so a large enabled XSAVE component
    ; cannot skip the Windows guard page. The final stack is 64-byte aligned;
    ; its first 32 bytes are the callback's required shadow space.
    mov r11, rsp
probe_page:
    sub r11, 1000h
    cmp r11, r10
    jb probe_final
    test byte ptr [r11], 0
    jmp probe_page
probe_final:
    mov rsp, r10
    test byte ptr [rsp], 0
    mov eax, crimson_crafting_state_low
    mov dword ptr [rsp+32], eax
    mov eax, crimson_crafting_state_high
    mov dword ptr [rsp+36], eax
    mov eax, crimson_crafting_state_mode
    mov dword ptr [rsp+40], eax
    ; XSAVE does not write every reserved byte. Clear the complete area first
    ; so XRSTOR cannot interpret uninitialized XCOMP_BV/reserved header bits.
    cld
    lea rdi, [rsp+64]
    mov ecx, crimson_crafting_state_bytes
    xor eax, eax
    rep stosb
    cmp dword ptr [rsp+40], 0
    je legacy_save
    mov eax, dword ptr [rsp+32]
    mov edx, dword ptr [rsp+36]
    xsave64 [rsp+64]
    jmp invoke_callback
legacy_save:
    fxsave64 [rsp+64]
invoke_callback:
    mov rcx, rbx
    call crimson_crafting_dispatch
    cmp dword ptr [rsp+40], 0
    je legacy_restore
    mov eax, dword ptr [rsp+32]
    mov edx, dword ptr [rsp+36]
    xrstor64 [rsp+64]
    jmp restore_registers
legacy_restore:
    fxrstor64 [rsp+64]
restore_registers:
    mov rsp, rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    popfq
    ; LEA preserves the restored flags. An indirect JMP from the discarded site
    ; cell preserves all registers and avoids a fabricated CET return address.
    lea rsp, [rsp+8]
    jmp qword ptr [rsp-8]
crimson_crafting_bridge ENDP
END
