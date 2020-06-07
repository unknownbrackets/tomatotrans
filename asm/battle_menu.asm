; Change the battle text length: Gimmick / Item / Run / Cool.
; Was at 0x08639BA8.

; We need a blank string afterward.
.org 0x087F0018
.fill 6

; Here we replace the length of 4 with 8
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
dw 0x087F0000

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
dw 0x087F0000

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
dw 0x087F0000

; This one is only Cool which was at 0x08639BB4.
.org 0x0804AE4A
mov r0,6
.org 0x0804EA68
dw 0x087F0018

; Another Cool only, have to reuse an and for cmp.
.org 0x0804090A
; Luckily, r4 seems to be free.
mov r4,6
mov r5,4
; This sets the Z flag, no need for another cmp.
and r1,r5
.org 0x0804091C
strb r4,[r0,5]
.org 0x0804095C
dw 0x087F0018

; Almost exactly like the above one.
.org 0x0804DCC6
; Very lucky we have r2 free here.
mov r2,6
mov r4,4
and r1,r4
.org 0x0804DCD8
strb r2,[r0,5]
.org 0x0804DD08
dw 0x087F0018
