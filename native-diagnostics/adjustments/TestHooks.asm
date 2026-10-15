EXTERN CenterSelectStub:PROC
EXTERN CenterSlotStub:PROC
.code
EXTERN LeftInputStub:PROC
EXTERN RightInputStub:PROC
EXTERN UpInputStub:PROC
EXTERN DownInputStub:PROC
EXTERN ResetInputStub:PROC

TestSelectReturn PROC
 ret
TestSelectReturn ENDP
TestSlotReturn PROC
 ret
TestSlotReturn ENDP
; Inputs: context, output[12]. Native RSI/EBP convention; report CL/EBP/CF.
RunSlotStubs PROC FRAME
 push rsi
 .pushreg rsi
 push rbp
 .pushreg rbp
 sub rsp, 40
 .allocstack 40
 .endprolog
 mov rsi,rcx
 mov cl,0
 mov ebp,2
 stc
 call CenterSelectStub
 mov byte ptr [rdx],cl
 setc byte ptr [rdx+8]
 call CenterSlotStub
 mov dword ptr [rdx+4],ebp
 setc byte ptr [rdx+9]
 add rsp,40
 pop rbp
 pop rsi
 ret
RunSlotStubs ENDP
RunInputStubs PROC FRAME
 push rsi
 .pushreg rsi
 sub rsp,32
 .allocstack 32
 .endprolog
 mov rsi,rcx
 mov cl,1
 stc
 call LeftInputStub
 mov byte ptr [rdx+0],cl
 setc byte ptr [rdx+8]
 mov cl,1
 stc
 call RightInputStub
 mov byte ptr [rdx+1],cl
 setc byte ptr [rdx+9]
 mov cl,1
 stc
 call UpInputStub
 mov byte ptr [rdx+2],cl
 setc byte ptr [rdx+10]
 mov cl,1
 stc
 call DownInputStub
 mov byte ptr [rdx+3],cl
 setc byte ptr [rdx+11]
 mov cl,1
 stc
 call ResetInputStub
 mov byte ptr [rdx+4],cl
 setc byte ptr [rdx+12]
 add rsp,32
 pop rsi
 ret
RunInputStubs ENDP
EXTERN SeatUiStub:PROC
TestSeatUiPass PROC
 mov eax,11
 ret
TestSeatUiPass ENDP
TestSeatUiBypass PROC
 mov eax,22
 ret
TestSeatUiBypass ENDP
RunSeatUiStub PROC FRAME
 push rdi
 .pushreg rdi
 sub rsp,32
 .allocstack 32
 .endprolog
 mov rdi,rcx
 stc
 call SeatUiStub
 setc byte ptr [rdx]
 add rsp,32
 pop rdi
 ret
RunSeatUiStub ENDP
END
