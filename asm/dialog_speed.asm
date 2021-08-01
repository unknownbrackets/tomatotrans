.org 0x0806CCBC
.region 0x0806CCF4-.,0x00
.func DialogDecideNextLetter
push r14
; Utility and dialog params.
ldr r3,=0x030018BC
ldr r1,=0x03004188

; Decrement the delay counter.
ldrb r0,[r1,15]
sub r0,r0,1
strb r0,[r1,15]
; Store also in the utility params for some reason.
strb r0,[r3,1]

; Are we ready to draw a new letter?
cmp r0,0
beq @@nextLetter
cmp r0,2
beq @@nextLetter

; No letter, set 0x20 bit in flags.
bl 0x0806CCA4
b @@return

@@nextLetter:
; Trigger the next letter.
bl 0x0806CD08
; And update the flags.
bl 0x0806CCF4

@@return:
pop r15
.pool
.endfunc
.endregion

.org 0x0806CC1C
.region 0x0806CCA4-.,0x00
.func DialogAddLetterAndBeep
push r4-r7,r14
; Prep the utility params and dialog params addresses.
ldr r5,=0x030018BC
ldr r4,=0x03004188

; Set the drawn flag and also write to utility params.
mov r0,1
strb r0,[r5,1]
strb r0,[r4,17]

; Draw the letter.
bl 0x0806E480

; Clear the 0x10 flag.
ldrb r0,[r4,14]
mov r1,0xEF
and r0,r1
strb r0,[r4,14]

; Check if the 0x04 flag (TICKEROFF) is set.
mov r3,0x04
and r3,r0 ; Sets ne if non-zero after and.
bne @@skipTicker

; Get the previous counter, which now might be 2.
ldrb r2,[r4,15]
cmp r2,2
beq @@skipSound

; Now get the reset value and reset the counter.
ldrb r0,[r4,16]
strb r0,[r4,15]

; This address holds the current sound type.
ldr r0,=0x03003E8B
ldrb r0,[r0]
; Store it in the utility params.
strb r0,[r5,1]

; If the sound effect is 0, skip.
cmp r0,0
beq @@skipSound

; Set the sound effect to play and play it.
ldrb r0,[r5,1]
strh r0,[r5,4]
bl 0x08065FC0

@@skipSound:
; Update flags with 0x20.
bl 0x0806CCA4

@@return:
pop r4-r7,r15

@@skipTicker:
; Update flags for next letter already.
bl 0x0806CCF4
b @@return
.pool
.endfunc
.endregion

; Ensure WAIT resets the counter.
.org 0x0806CF2C
mov r3,0
strb r3,[r6,15]
pop r4-r7,r15

; Same for CONTINUE.
.org 0x0806CFA6
mov r3,0
strb r3,[r6,15]
pop r4,r15

; CLEAR...
.org 0x0806CFBA
mov r3,0
strb r3,[r6,15]
pop r15

; SIMPLEWAIT...
.org 0x0806D0E6
mov r3,0
strb r3,[r6,15]
pop r4,r15

; PAUSE...
.org 0x0806D820
mov r3,0
strb r3,[r6,15]
pop r4,r15

; SETPAUSE...
.org 0x0806D8FE
mov r3,0
strb r3,[r6,15]
pop r15

; PAUSEBREAK...
.org 0x0806D96A
mov r3,0
strb r3,[r6,15]
pop r4-r6,r15
