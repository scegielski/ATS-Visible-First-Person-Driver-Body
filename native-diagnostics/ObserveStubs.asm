EXTERN gMapWidgetOriginal:QWORD
EXTERN CaptureMapWidget:PROC
EXTERN gMenuModeOriginal:QWORD
EXTERN gMenuSurfaceOriginal:QWORD
EXTERN CaptureMenuMode:PROC
EXTERN CaptureMenuSurface:PROC
EXTERN gDriverOriginal:QWORD
EXTERN gEngineOriginal:QWORD
EXTERN gHornControlOriginal:QWORD
EXTERN CaptureHornBox:PROC
EXTERN CaptureEngineRequest:PROC
EXTERN gAdvanceOriginal:QWORD
EXTERN CaptureDriver:PROC
EXTERN CaptureAdvance:PROC
EXTERN gBoneOriginal:QWORD
EXTERN CaptureBone:PROC
EXTERN PostBone:PROC
EXTERN gRecenterOriginal:QWORD
EXTERN CaptureNativeRecenter:PROC
EXTERN gHeadOriginal:QWORD
EXTERN CaptureHeadViews:PROC
EXTERN CaptureXRContext:PROC
EXTERN gXRActionAttachOriginal:QWORD
EXTERN PrepareControllerActionAttach:PROC
EXTERN gRadioInputOriginal:QWORD
EXTERN CaptureRadioInput:PROC
EXTERN gWipersOriginal:QWORD
EXTERN gLightsOriginal:QWORD
EXTERN gSignalLeftOriginal:QWORD
EXTERN gSignalRightOriginal:QWORD
EXTERN CaptureSignalRequest:PROC
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
; Mid-function sites have RSP aligned to 16, unlike function-entry observers.
SIGNAL_STUB MACRO name, original, side
name PROC FRAME
 pushfq
 .allocstack 8
 sub rsp, 0c8h
 .allocstack 0c8h
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
 mov r8d, ecx
 mov edx, side
 mov rcx, rdi
 call CaptureSignalRequest
 mov byte ptr [rsp+28h], al ; only CL is intentionally changed
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
 add rsp, 0c8h
 popfq
 jmp qword ptr [original]
name ENDP
ENDM
RadioInputObserveStub PROC FRAME
 pushfq
 .allocstack 8
 sub rsp, 0c8h
 .allocstack 0c8h
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
 mov rcx, rsi
 call CaptureRadioInput
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
 add rsp, 0c8h
 popfq
 jmp qword ptr [gRadioInputOriginal]
RadioInputObserveStub ENDP
NativeRecenterObserveStub PROC FRAME
 pushfq
 .allocstack 8
 sub rsp, 0c8h
 .allocstack 0c8h
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
 mov edx, ecx
 mov rcx, r12
 call CaptureNativeRecenter
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
 add rsp, 0c8h
 popfq
 jmp qword ptr [gRecenterOriginal]
NativeRecenterObserveStub ENDP
HeadObserveStub PROC FRAME
 pushfq
 .allocstack 8
 sub rsp, 0c8h
 .allocstack 0c8h
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
 mov rcx, rsi ; validated OpenXR owner at this exact hook site
 call CaptureXRContext
 mov rcx, [rbp-30h] ; returned view array
 mov edx, [rbp+110h] ; returned view count
 mov r8, [rsp+140h] ; original rsp+70h: view-state flags
 mov r9d, [rsp+58h] ; unchanged xrLocateViews result
 lea rax, [rbp-60h] ; locate info, including tracking space and time
 mov [rsp+20h], rax
 call CaptureHeadViews
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
 add rsp, 0c8h
 popfq
 jmp qword ptr [gHeadOriginal]
HeadObserveStub ENDP
XRActionAttachStub PROC FRAME
 pushfq
 .allocstack 8
 sub rsp, 0c8h
 .allocstack 0c8h
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
 mov rcx, rsi
 mov rdx, [rsp+30h]
 call PrepareControllerActionAttach
 mov [rsp+30h], rax ; replace only attachment-info pointer
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
 add rsp, 0c8h
 popfq
 jmp qword ptr [gXRActionAttachOriginal]
XRActionAttachStub ENDP

SIGNAL_STUB WipersRequestStub, gWipersOriginal, 4
SIGNAL_STUB LightsRequestStub, gLightsOriginal, 3
SIGNAL_STUB SignalLeftRequestStub, gSignalLeftOriginal, 1
SIGNAL_STUB SignalRightRequestStub, gSignalRightOriginal, 2
EngineRequestObserveStub PROC FRAME
 SAVE_STATE
 mov r9, [rsp+0c8h] ; original caller, callback only
 call CaptureEngineRequest
 test al, al
 jnz EngineRequestDeferred
 RESTORE_STATE
 jmp qword ptr [gEngineOriginal]
EngineRequestDeferred:
 mov qword ptr [rsp+58h], 0
 RESTORE_STATE
 ret
EngineRequestObserveStub ENDP
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
 pushfq
 .allocstack 8
 sub rsp, 100h
 .allocstack 100h
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
 mov [rsp+0c0h], rcx ; output survives original call
 mov [rsp+0c8h], rdx ; private model bounds
 mov eax, [rsp+130h] ; fifth argument at original rsp+28h
 mov [rsp+0e0h], eax
 mov [rsp+20h], eax
 call CaptureBone
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
 push qword ptr [rsp+100h]
 popfq
 call qword ptr [gBoneOriginal]
 ; Preserve the original function's outputs across the render-only callback.
 mov [rsp+58h], rax
 pushfq
 pop rax
 mov [rsp+100h], rax
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
 mov rcx, [rsp+0c0h]
 mov rdx, [rsp+0c8h]
 mov r8d, [rsp+0e0h]
 call PostBone
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
 add rsp, 100h
 popfq
 ret
BoneObserveStub ENDP
HornControlObserveStub PROC FRAME
 pushfq
 .allocstack 8
 sub rsp, 0c8h
 .allocstack 0c8h
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
 call CaptureHornBox
 mov byte ptr [rsp+30h], al
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
 add rsp, 0c8h
 popfq
 jmp qword ptr [gHornControlOriginal]
HornControlObserveStub ENDP
MenuModeObserveStub PROC FRAME
 pushfq
 .allocstack 8
 sub rsp, 0c8h
 .allocstack 0c8h
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
 movzx ecx, dil
 call CaptureMenuMode
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
 add rsp, 0c8h
 popfq
 jmp qword ptr [gMenuModeOriginal]
MenuModeObserveStub ENDP
MenuSurfaceObserveStub PROC FRAME
 pushfq
 .allocstack 8
 sub rsp, 0c8h
 .allocstack 0c8h
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
 mov rcx, rbp
 call CaptureMenuSurface
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
 add rsp, 0c8h
 popfq
 jmp qword ptr [gMenuSurfaceOriginal]
MenuSurfaceObserveStub ENDP
MapWidgetObserveStub PROC FRAME
 SAVE_STATE
 call CaptureMapWidget
 RESTORE_STATE
 jmp qword ptr [gMapWidgetOriginal]
MapWidgetObserveStub ENDP

END
