EXTERN gDriverOriginal:QWORD
EXTERN gAdvanceOriginal:QWORD
EXTERN CaptureDriver:PROC
EXTERN CaptureAdvance:PROC
EXTERN gBoneOriginal:QWORD
EXTERN CaptureBone:PROC
.code
SAVE_STATE MACRO
 pushfq
 .allocstack 8
 sub rsp, 0c0h
 .allocstack 0c0h
 .endprolog
 mov [rsp+58h], rax
 mov [rsp+28h], rcx
 mov [rsp+30h], rdx
 mov [rsp+38h], r8
 mov [rsp+40h], r9
 mov [rsp+48h], r10
 mov [rsp+50h], r11
 movdqu [rsp+60h], xmm0
 movdqu [rsp+70h], xmm1
 movdqu [rsp+80h], xmm2
 movdqu [rsp+90h], xmm3
 movdqu [rsp+0a0h], xmm4
 movdqu [rsp+0b0h], xmm5
ENDM
RESTORE_STATE MACRO
 movdqu xmm0, [rsp+60h]
 movdqu xmm1, [rsp+70h]
 movdqu xmm2, [rsp+80h]
 movdqu xmm3, [rsp+90h]
 movdqu xmm4, [rsp+0a0h]
 movdqu xmm5, [rsp+0b0h]
 mov r11, [rsp+50h]
 mov r10, [rsp+48h]
 mov r9, [rsp+40h]
 mov r8, [rsp+38h]
 mov rdx, [rsp+30h]
 mov rcx, [rsp+28h]
 mov rax, [rsp+58h]
 add rsp, 0c0h
 popfq
ENDM
DriverObserveStub PROC FRAME
 SAVE_STATE
 call CaptureDriver
 RESTORE_STATE
 jmp qword ptr [gDriverOriginal]
DriverObserveStub ENDP
AdvanceObserveStub PROC FRAME
 SAVE_STATE
 mov r9, [rsp+0c8h] ; unmodified game's return address
 lea rax, [rsp+80h] ; callback may replace only the animation float
 mov [rsp+20h], rax ; fifth argument, outside saved registers
 call CaptureAdvance
 RESTORE_STATE
 jmp qword ptr [gAdvanceOriginal]
AdvanceObserveStub ENDP
BoneObserveStub PROC FRAME
 SAVE_STATE
 mov eax, [rsp+0f0h] ; original fifth argument: bone count
 mov [rsp+20h], eax
 call CaptureBone
 RESTORE_STATE
 jmp qword ptr [gBoneOriginal]
BoneObserveStub ENDP
END
