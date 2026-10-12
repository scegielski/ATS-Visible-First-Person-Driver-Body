EXTERN SignalLeftRequestStub:PROC
EXTERN SignalRightRequestStub:PROC
.code
SignalTestLeft PROC FRAME
 push rdi
 .pushreg rdi
 sub rsp, 20h
 .allocstack 20h
 .endprolog
 mov rdi, rcx
 mov rcx, rdx
 jmp SignalLeftRequestStub
SignalTestLeft ENDP
SignalTestRight PROC FRAME
 push rdi
 .pushreg rdi
 sub rsp, 20h
 .allocstack 20h
 .endprolog
 mov rdi, rcx
 mov rcx, rdx
 jmp SignalRightRequestStub
SignalTestRight ENDP
SignalTestContinuation PROC
 mov [r9], rdi
 mov [r9+8], rdx
 mov [r9+16], r8
 mov rax, rcx
 add rsp, 20h
 pop rdi
 ret
SignalTestContinuation ENDP
END
