EXTERN dll_callback_impl:PROC
EXTERN new_gold_value_buffer:DWORD

.code

dll_callback PROC EXPORT
    push rbx
	mov rbx, r9  ; Save value

    ; Calling parameters
    mov rcx, r9
    mov rdx, rdx
    mov r8, r8
    sub rsp, 8 * 3 + 4 ; Allocate shadow stack
    call dll_callback_impl
    add rsp, 8 * 3 + 4

    ; Prepare overwritten code
    lea rdx, new_gold_value_buffer
    mov rcx, rbx
    pop rbx
    
    ret
dll_callback endp

end
