; Many messages are copied to a fixed size buffer at 0x03000BFC, but we want longer.
; Really, they don't need to be there - we can read directly from the ROM.
; We edit the func to use a pointer in that buffer for long strings, instead.

; 0804A168 is pretty long so just rewriting the relevant part.
; All regs are actually free at this point, luckily.

; Original buffer:
; uint8_t align = { LEFT = 0, CENTER = 1, RIGHT = 2 }; // Doesn't work with VWF.
; uint8_t length;
; char str[0x1E]; // We store pointer at &str[2] for alignment reasons.

; Before the literal pool...
.org 0x0804A4E0
.area 0x0804A4F8-.,0x00
; Load the existing 0x03000BFC from the literal pool.
ldr r0,[r15,0x0804A50C-.-4]

; Read length into r7 and skip on length zero.
ldrb r7,[r0,1]
cmp r7,0
beq 0x0804A54C

; Load the font draw params struct - 0x030018BC.
ldr r5,[r15,0x0804A580-.-4]

; Now the alignment flag.
ldrb r1,[r0,0]
mov r6,0
; Used for alignment, not compatible with VWF really...
; Kept to avoid breaking other strings.
mov r2,0x1A

; 0 = left, 1 = center, 2 = right.
cmp r1,1
beq @alignCenterDesc
bgt @alignRightDesc
b @alignDoneDesc
.endarea

;After the literal pool...
.org 0x0804A510
.area 0x0804A54C-.,0x00
@alignCenterDesc:
; xpos = (0x1A - length) / 2
mov r6,r2
sub r6,r7
lsr r6,r6,1
b @alignDoneDesc

@alignRightDesc:
; xpos = 0x1A - length
mov r6,r2
sub r6,r7

@alignDoneDesc:

; This is the literal pool address for 0x06000820.
ldr r1,[r15,0x0804A588-.-4]
; Set the dest parameter for drawing.
lsl r6,r6,5
add r1,r6
str r1,[r5,8]

; Now, if length is > 0x1A, read as a pointer.  r2 still has 0x1A.
cmp r7,r2
bgt @@largeText

; Use the buffer as the source for < 0x1A.
add r0,r0,2
b @@drawText

@@largeText:
ldr r0,[r0,4]

@@drawText:
; Actually store the string pointer.
str r0,[r5,12]

mov r1,r7
; This calculates the length less padding.  So we don't draw too many spaces at the end.
; Expects r0=str, r1=maxLength
bl 0x080484A8
strb r0,[r5,5]

; Draw the description.
bl 0x8071748

; There's a lot of blank space now, skip it.
b 0x0804A54C
.endarea
