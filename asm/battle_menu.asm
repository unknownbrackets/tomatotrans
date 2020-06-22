; Change the battle text length: Gimmick / Item / Run / Cool.
; Was at 0x08639BA8.

; We need a blank string afterward.
.org org(BattleMenuText) + 24
.fill 6

; Here we replace the length of 4 with 6.
.org 0x0804A636
mov r6,6
.org 0x0804A642
add r0,r5,r6
.org 0x0804A652
add r0,12
.org 0x0804A66A
add r0,18
.org 0x0804A69E
add r0,24
.org 0x0804A688
dw org(BattleMenuText)

; Basically the same pattern here, but skips Item.
.org 0x0804EC12
mov r6,6
.org 0x0804EC20
add r0,12
.org 0x0804EC38
add r0,18
.org 0x0804EC5E
add r0,24
.org 0x0804EC4C
dw org(BattleMenuText)

; And one more time.
.org 0x0804FAA6
mov r7,6
.org 0x0804FAB2
add r0,r6,r7
.org 0x0804FAC2
add r0,12
.org 0x0804FADE
add r0,18
.org 0x0804FB1C
dw org(BattleMenuText)

; This one is only Cool which was at 0x08639BB4.
.org 0x0804AE4A
mov r0,6
.org 0x0804EA68
dw org(BattleMenuText) + 18

; Another Cool only, have to reuse an and for cmp.
.org 0x0804090A
.area 0x0804091E-.,0x00
mov r5,4
; This sets the Z flag, no need for another cmp.
and r1,r5
beq @@skipRed
; Set color to red.
bl 0x08071A4C
@@skipRed:
ldr r0,[0x08040958] ; 0x030018BC
ldr r1,[0x0804095C] ; 0x08639BB4 (we replace this)
str r1,[r0,12]
mov r2,6
strb r2,[r0,5h]
.endarea
.org 0x0804095C
dw org(BattleMenuText) + 18

; Almost exactly like the above one.
.org 0x0804DCC6
.area 0x0804DCDA-.,0x00
mov r4,4
and r1,r4
beq @@skipRed
; Set color to red.
bl 0x08071A4C
@@skipRed:
ldr r0,[0x0804DD04] ; 0x030018BC
ldr r1,[0x0804DD08] ; 0x08639BB4 (we replace this)
str r1,[r0,12]
mov r2,6
strb r2,[r0,5h]
.endarea
.org 0x0804DD08
dw org(BattleMenuText) + 18

.ifdef NameMaxLength
; This is part of the larger 0804A930, which draws the in battle stats.
; We just adjust to show longer names.
.org 0x0804A97E
.area 0x0804A9A2-.,0x00
; At this point: r0=which, 1=FREE, r2=FREE, r3=FREE, r4=FREE, r9=0x030018BC, r10=y
; Requires as output: r4=yByteOffset - do that right away.
mov r4,r10
lsl r4,r4,11

; Grab the utility drawing params.
mov r3,r9
ldr r1,[0x0804AA48] ; 0x06000D00
add r1,r1,r4
str r1,[r3,8]

; Okay, we still have r0=which char.  Draw.
bl CopyCharName8x8ToVRAM

b 0x0804A9A2
.endarea
.endif
