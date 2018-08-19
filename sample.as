;file sample.as
.entry  LENGTH
.extern W
MAIN:   mov r3, LENGTH
LOOP:   jmp L1(#-1,r6)
K:        prn #-5
        bne W(r4,r5)
        sub r1, r4
        bne L3
L1:     inc K
        bne LOOP(K,W)
END:    stop ;aaa
STR:    .string "abcdef"
LENGTH: .data 6,-9,15
.entry  LOOP
.extern L3