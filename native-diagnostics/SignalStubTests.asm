EXTERN HeadObserveStub:PROC
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
HeadTestEntry PROC FRAME
 push rbp
 .pushreg rbp
 sub rsp, 300h
 .allocstack 300h
 .endprolog
 lea rbp, [rsp+180h]
 mov [rbp-30h], rcx
 mov [rbp+110h], edx
 mov [rsp+70h], r8
 mov dword ptr [rbp-60h], 6
 mov qword ptr [rbp-48h], 42
 mov qword ptr [rbp-40h], 456
 mov eax, r9d
 jmp HeadObserveStub
HeadTestEntry ENDP
HeadTestContinuation PROC
 add rsp, 300h
 pop rbp
 ret
HeadTestContinuation ENDP
END
