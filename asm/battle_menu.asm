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
.region 0x0804A9A2-.,0x00
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
.endregion
.endif

; We rewrite this function to save some bytes and center "Cool".
; It shows "Cool" on the right side in the battle menu.
.org 0x0804ADE0
.region 0x0804AE70-.,0x00
.func BattleUpdateCoolIndicator
push r14
add sp,-24

; Prepare some of the struct in sp before the branch.
mov r2,0
; This was originally two strh, but we can zero both at once.
str r2,[sp,4]
mov r1,6
mov r0,sp
strh r1,[r0,8]
strh r1,[r0,10]
ldr r3,=0x060073B0
str r3,[sp,12]
; Also originally two strh.
str r2,[sp,16]
strh r1,[r0,20]
strh r1,[r0,22]

ldr r3,=0x03000BAD
ldrb r3,[r3,0]
cmp r3,1
bne @@displayCool

; I'm not sure exactly what this bl is doing, maybe icons?  Does a DMA3.
ldr r3,=0x085E6938
str r3,[sp,0]
; Takes r0=sp, already prepared earlier.
bl 0x080400C4
b @@return

@@displayCool:
ldr r3,=0x085E68F0
str r3,[sp,0]
; Takes r0=sp, already prepared earlier.
bl 0x080400C4

ldr r0,=org(BattleMenuText) + 18
mov r1,6
mov r2,1
bl Calc8x8PixelWidth

; Okay, now we can draw centered - r0=str, r1=len, r2=pixel width.
ldr r3,=0x030018BC
strb r1,[r3,5]
ldr r1,=0x06000EC0
str r1,[r3,8]
str r0,[r3,12]

; Center in the 4 tiles available.
mov r0,4
; Move slightly left.
add r2,r2,1
bl CopyString8x8CenterR0

@@return:
add sp,24
pop r0
bx r0
.pool
.endfunc
.endregion
