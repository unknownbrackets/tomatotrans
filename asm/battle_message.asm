; Many messages are copied to a fixed size buffer at 0x03000BFC, but we want longer.
; Really, they don't need to be there - we can read directly from the ROM.
; We edit the func to use a pointer in that buffer for long strings, instead.

; 0804A168 is pretty long so just rewriting the relevant part.
; All regs are actually free at this point, luckily (r4-r7 were saved.)

; Original buffer:
; uint8_t align = { LEFT = 0, CENTER = 1, RIGHT = 2 }; // Doesn't work with VWF.
; uint8_t length;
; char str[0x1E]; // We store pointer at &str[2] for alignment reasons.

; Before the literal pool...
.org 0x0804A4E0
.area 0x0804A4F8-.,0x00
; Load the existing buffer pointer from the literal pool.
ldr r4,[0x0804A50C] ; 0x03000BFC
; Load the font draw params struct.
ldr r5,[0x0804A580] ; 0x030018BC

; Prepare a flag for the Calc8x8PixelWidth call.
mov r2,1

; Read max length into r1 and compare to 0x1A.
ldrb r1,[r4,1]
mov r6,0x1A
cmp r1,r6
bgt @@largeText

; Smaller, so embedded.
add r0,r4,2
b @stringReady

@@largeText:
ldr r0,[r4,4]
b @stringReady
.endarea

;After the literal pool...
.org 0x0804A510
.area 0x0804A54C-.,0x00
@stringReady:
; Calculate the actual width of the string, including stripping trailing spaces.
bl Calc8x8PixelWidth

; Skip if zero length.
cmp r2,0
beq 0x0804A54C

; This is the literal pool address for the VRAM dest to draw to.
ldr r7,[0x0804A588] ; 0x06000820
; Make r6 the availWidth, and convert both to bytes for easy math.
lsl r6,r6,5
lsl r2,r2,2

; Now let's check the alignment flag.
ldrb r3,[r4,0]
; 0 = left, 1 = center, 2 = right.
cmp r3,1
beq @@alignCenter
bgt @@alignRight
b @@alignDone

@@alignCenter:
; xpos = (availWidth - actualWidth) / 2
sub r6,r6,r2
lsr r6,1
add r7,r6
b @@alignDone

@@alignRight:
; xpos = availWidth - actualWidth
sub r6,r6,r2
add r7,r6

@@alignDone:
; Okay, that's our draw destination.
str r7,[r5,8]
; Then store the string pointer and length.
str r0,[r5,12]
strb r1,[r5,5]
; Draw the message.
bl CopyString8x8ToVRAM

; There's a lot of blank space now, skip it.
b 0x0804A54C
.endarea
