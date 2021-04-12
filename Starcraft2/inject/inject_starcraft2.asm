.code

EXTERN hook_NtQueryInformationThread:PROC

; Anti Cheat checks first byte!
; First byte of this must match the first byte of NtQueryInformationThread
asm_wrap_NtQueryInformationThread PROC EXPORT
    mov r10, rcx ; = First instruction in NtQueryInformationThread, here it does nothing
    jmp hook_NtQueryInformationThread
asm_wrap_NtQueryInformationThread ENDP


EXTERN fn_call_wrapper:PROC

; uint64_t call_in_main_section_context(uint64_t* func, uint64_t arg1, uint64_t arg2, uint64_t arg3);
call_in_main_section_context PROC EXPORT
    sub rsp, 48h
    mov rax, rcx

    ; shift parameters
    mov rcx, rdx
    mov rdx, r8
    mov r8, r9

    ; fake call
    mov r9, fn_call_wrapper
    mov r9, [r9]
    add r9, 5
    push r9
    jmp rax
    ; fn_call_wrapper:
    ; call <anything>
    ; add rsp, 0x48 <- jmp returns here
    ; ret
call_in_main_section_context ENDP

END
