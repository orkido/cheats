.code

EXTERN hook_NtQueryInformationThread:PROC

; Anti Cheat checks first byte!
; First byte of this must match the first byte of NtQueryInformationThread
asm_wrap_NtQueryInformationThread PROC EXPORT
    mov r10, rcx ; = First instruction in NtQueryInformationThread, here it does nothing
    jmp hook_NtQueryInformationThread
asm_wrap_NtQueryInformationThread ENDP

END
