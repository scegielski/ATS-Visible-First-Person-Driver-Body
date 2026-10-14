EXTERN NativeRecenterObserveStub:PROC
EXTERN XRActionAttachStub:PROC
EXTERN HeadObserveStub:PROC
EXTERN RadioInputObserveStub:PROC
EXTERN WipersRequestStub:PROC
EXTERN LightsRequestStub:PROC
EXTERN SignalLeftRequestStub:PROC
EXTERN SignalRightRequestStub:PROC
.code
RadioInputTest PROC FRAME
 push rsi
 .pushreg rsi
 sub rsp, 20h
 .allocstack 20h
 .endprolog
 mov rsi, rcx
 mov rcx, rdx
 jmp RadioInputObserveStub
RadioInputTest ENDP
RadioInputContinuation PROC
 mov rax, rcx
 add rsp, 20h
 pop rsi
 ret
RadioInputContinuation ENDP
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
WipersTest PROC FRAME
 push rdi
 .pushreg rdi
 sub rsp, 20h
 .allocstack 20h
 .endprolog
 mov rdi, rcx
 mov rcx, rdx
 jmp WipersRequestStub
WipersTest ENDP
LightsTest PROC FRAME
 push rdi
 .pushreg rdi
 sub rsp, 20h
 .allocstack 20h
 .endprolog
 mov rdi, rcx
 mov rcx, rdx
 jmp LightsRequestStub
LightsTest ENDP
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
RecenterTestEntry PROC FRAME
 push r12
 .pushreg r12
 sub rsp, 20h
 .allocstack 20h
 .endprolog
 mov r12, rcx
 mov rcx, rdx
 jmp NativeRecenterObserveStub
RecenterTestEntry ENDP
RecenterTestContinuation PROC
 mov rax, rcx
 add rsp, 20h
 pop r12
 ret
RecenterTestContinuation ENDP
XRActionAttachTest PROC FRAME
 push rsi
 .pushreg rsi
 sub rsp, 20h
 .allocstack 20h
 .endprolog
 mov rsi, rcx
 mov rcx, 42
 jmp XRActionAttachStub
XRActionAttachTest ENDP
XRActionAttachContinuation PROC
 mov rax, rdx
 cmp rcx, 42
 je AttachArgsOK
 xor eax, eax
AttachArgsOK:
 add rsp, 20h
 pop rsi
 ret
XRActionAttachContinuation ENDP
END
