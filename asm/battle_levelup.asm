; This just tweaks it so the numbers are a bit farther to the right.

.org 0x08050D10
dw 0x06001560
.org 0x08050D14
dw 0x06001960
.org 0x08050D78
dw 0x06000C20
.org 0x08050DEC
dw 0x06000C20

.ifdef NameMaxLength
; This is part of the larger 08050BB8, where it shows the "leveled up" message.
; We adjust both the position of "leveled up" and the name length.
.org 0x08050C62
.area 0x08050C8A-.,0x00
; At this point: r0=FREE, r1=FREE, r2=FREE, r3=FREE
; r4=0x030018BC (keep), r5=FREE, r6=FREE, r7=ptrToCharNum, r9=0x03001EDC
; Requires r6 become 4 (or whatever length "up" is.)
mov r6,4
; Get the character number into r0.
ldrb r0,[r7]

; Now draw it.
ldr r5,[0x08050CF0] ; 0x06000C20
str r5,[r4,8]
bl CopyCharName8x8ToVRAM

mov r0,11
strb r0,[r4,5]

; As output, [r4,1] is the pixel width.  We'll draw leveled up after that.
ldrb r0,[r4,1]
lsl r0,r0,2
; Add 5 pixels as a space.
add r0,20

add r5,r5,r0
str r5,[r4,8]

ldr r0,[0x08050CF4] ; 0x08639C93
str r0,[r4,12]

bl CopyString8x8ToVRAM
.endarea
.endif
