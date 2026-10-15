EXTERN gOverrideContext:QWORD
EXTERN gOverrideSlot:DWORD
EXTERN gInputMask:BYTE
EXTERN gLeftOriginal:QWORD
EXTERN gRightOriginal:QWORD
EXTERN gUpOriginal:QWORD
EXTERN gDownOriginal:QWORD
EXTERN gResetOriginal:QWORD

EXTERN gSelectOriginal:QWORD
EXTERN gSlotOriginal:QWORD
EXTERN gSyntheticSeatContext:QWORD
EXTERN gSeatUiOriginal:QWORD
EXTERN gSeatApplyReturn:QWORD
.code
; At the selector fragment, RSI is context and CL is near-selection result.
; Force the near branch only during our scoped call for the same context.
CenterSelectStub PROC
    pushfq
    cmp rsi, qword ptr [gOverrideContext]
    jne select_pass
    test rsi, rsi
    jz select_pass
    popfq
    mov cl, 1
    jmp qword ptr [gSelectOriginal]
select_pass:
    popfq
    jmp qword ptr [gSelectOriginal]
CenterSelectStub ENDP
; Both native selections converge here; EBP holds the physical slot.
CenterSlotStub PROC
    pushfq
    cmp rsi, qword ptr [gOverrideContext]
    jne slot_pass
    test rsi, rsi
    jz slot_pass
    popfq
    mov ebp, dword ptr [gOverrideSlot]
    jmp qword ptr [gSlotOriginal]
slot_pass:
    popfq
    jmp qword ptr [gSlotOriginal]
CenterSlotStub ENDP
LeftInputStub PROC
    pushfq
    cmp rsi, qword ptr [gOverrideContext]
    jne Left_pass
    test rsi, rsi
    jz Left_pass
    test byte ptr [gInputMask], 1
    setnz cl
Left_pass:
    popfq
    jmp qword ptr [gLeftOriginal]
LeftInputStub ENDP
RightInputStub PROC
    pushfq
    cmp rsi, qword ptr [gOverrideContext]
    jne Right_pass
    test rsi, rsi
    jz Right_pass
    test byte ptr [gInputMask], 2
    setnz cl
Right_pass:
    popfq
    jmp qword ptr [gRightOriginal]
RightInputStub ENDP
UpInputStub PROC
    pushfq
    cmp rsi, qword ptr [gOverrideContext]
    jne Up_pass
    test rsi, rsi
    jz Up_pass
    test byte ptr [gInputMask], 4
    setnz cl
Up_pass:
    popfq
    jmp qword ptr [gUpOriginal]
UpInputStub ENDP
DownInputStub PROC
    pushfq
    cmp rsi, qword ptr [gOverrideContext]
    jne Down_pass
    test rsi, rsi
    jz Down_pass
    test byte ptr [gInputMask], 8
    setnz cl
Down_pass:
    popfq
    jmp qword ptr [gDownOriginal]
DownInputStub ENDP
ResetInputStub PROC
    pushfq
    cmp rsi, qword ptr [gOverrideContext]
    jne Reset_pass
    test rsi, rsi
    jz Reset_pass
    test byte ptr [gInputMask], 16
    setnz cl
Reset_pass:
    popfq
    jmp qword ptr [gResetOriginal]
ResetInputStub ENDP
; RDI is the native seat menu object. Bypass only the UI tail for our private object.
SeatUiStub PROC
 pushfq
 cmp rdi, qword ptr [gSyntheticSeatContext]
 jne seat_ui_pass
 test rdi,rdi
 jz seat_ui_pass
 popfq
 jmp qword ptr [gSeatApplyReturn]
seat_ui_pass:
 popfq
 jmp qword ptr [gSeatUiOriginal]
SeatUiStub ENDP
END
