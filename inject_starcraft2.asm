ifndef X64
.model flat, C
endif

.code

EXTERN hook2_NtQueryInformationThread@20:PROC

; Anti Cheat checks first byte
hook_NtQueryInformationThread@20 PROC FAR EXPORT
    mov eax, 25h
    jmp hook2_NtQueryInformationThread@20
hook_NtQueryInformationThread@20 ENDP

END
