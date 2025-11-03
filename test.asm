; ======================
; NAN-ISA Tokenizer Test
; ======================

start:
    MOV A, B                ; reg -> reg
    MOV C, [DI]             ; reg <- reg-indirect
    MOV [SI+4], D           ; reg-indirect + displacement
    MOV word [0x2000], 0x1234   ; absolute with explicit size (OEX handled)
    MOV [BP+DI*2], A        ; SIB addressing
    MOV [BP+DI*2+8], B      ; SIB + displacement

    ADD A, C                ; simple ALU
    ADC B, [SI]             ; extended opcode
    SUB D, [0x3000]         ; absolute memory
    INC A                   ; unary extended
    DEC B
    MUL C, A
    DIV D, B
    NOT [DI+2]              ; unary, memory + displacement

    AND A, B
    OR  C, D
    XOR [SI], 0xFF
    CMP A, [DI]

; Control flow / labels
loop_start:
    JZ end_loop
    JNZ loop_start
    JC  carry_handler
    JNC no_carry
    JA  greater
    JBE less_or_equal

carry_handler:
    SUB A, 1
    RET

no_carry:
    INC B
    JMP loop_start

greater:
    MOV D, 0x10
    JLE loop_start

end_loop:
    NOP
    HLT

; Stack ops
    PUSH A
    PUSH [SI+6]
    POP B
    POP [DI]

; I/O instructions
    INB  [0x80]
    OUTB [0x80], A

; Load effective address
    LEA C, [BP+SI*4+12]
