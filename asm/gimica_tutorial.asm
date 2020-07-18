@TutorialTextLength equ 48

; 08091268 renders the tutorial messaging and breaks between.  We make messages a bit longer.
; To do this, we also rewrite some to time messages based on actual length.

; Here it's just 4 frames per letter, and this is the max letters.
.org 0x08091294
cmp r7,@TutorialTextLength - 1

; Here the game pointlessly buffers a pointer over itself, so we have space.
; We only need r2 to be the value at [ [0x03000100] + 1 ] and r7=0.  All others under r7 are free.
.org 0x080912DC
.area 0x0809130E-.,0x00
; Grab the pointers, and the current index.
ldr r0,[0x0809140C] ; 0x0864B1F8
ldr r1,[0x08091408] ; 0x0300010C
ldrb r1,[r1]
lsl r1,r1,2
; Finally, load the string itself.
ldr r0,[r0,r1]

; Now calculate the length into r4 (which is free.)
mov r1,@TutorialTextLength
bl CalcFixedStringLength
mov r4,r1

; The [0x03000100] pointer was saved here, so read it back.
mov r0,r12
ldr r2,[r0,12]
ldrb r2,[r2,1]
strb r2,[r0,1]
b 0x0809130E
.endarea

; This was a bne, but make it a bls in case we go longer.
.org 0x0809131E
cmp r0,r4
bls 0x080913A0
; But this must always be the fixed length + 1.
.org 0x08091334
mov r1,@TutorialTextLength + 1

.org 0x080913C8
cmp r0,r4
.org 0x080913EE
cmp r0,r4

; It used to pointlessly mask a constant, we replace this with a simple constant with the clear value.
.org 0x080913D8
ldr r0,[0x08091410] ; 0x001B0100
nop
.org 0x08091410
dw 0x001B0100
