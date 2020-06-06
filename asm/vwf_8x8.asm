; The 8x8 VWF font is already partially implemented in the game.
; The drawing func (080713D0) doesn't blend correctly, so it's not used.
; widthIndex is stored as 0xFD (means normal char) in 0806FA50.
; This patches both of those functions.

; Inside 0806FA50, we just rewrite some registers to save the right value.
; Don't clobber code in r0 so we can avoid reloading it.  Switches r0 to r2.
.org 0x0806FA84
ldr r2,[r3,12]
ldrb r1,[r2]
strb r1,[r3,5]
add r2,1
str r2,[r3,12]
; Here the char is used to get the bitmap offset, again switch to r2.
.org 0x0806FA9A
lsl r2,r0,4
.skip 2 ; ldr for 0x08648748, bitmap address.
add r2,r1
str r2,[r3,8]
; Still store the code as 0xFD (processed later to draw it.)
mov r2,0xFD
strb r2,[r3,1]
; This used to be a ldr to r0 from the code (0xFD above) to store as the widthIndex.
; We just nop it out since r0 is already the correct widthIndex.
.org 0x0806FAA8
nop

; This function renders a single character, and copies it to tile memory.
; The original handled VWF offsets incorrectly, so we're rewriting it here.
org 0x080713D0
.area 0x080715A4-.,0x00
.func CopyChar8x8ToVRAM
mov r0,r8
mov r1,r9
mov r2,r10
push r0-r2,r4-r7,r14
ldr r0,=0x9999
push r0

; Trade around some state values.
ldr r7,=0x030018BC
ldr r1,=0x03001AD2
ldrb r0,[r7,4]
strb r0,[r7,1]
strb r0,[r1]
; r0 = charBitmap (we keep this in r0 for a bit.)
ldr r0,[r7,8]
str r0,[r7,12]

; Append charBitmap and current [r7,4] to stack0608.
ldr r5,=0x03000608
ldr r6,=0x03000604
ldr r1,[r6]
lsl r2,r1,2
add r2,r5
add r1,2
str r1,[r6]
ldr r1,[r7,4]
stmia r2!,r0,r1
; Remember for later.
mov r9,r5
mov r10,r6

; Write 64 bytes (32 halfwords) to workarea.
ldr r1,=0x040000D4
ldr r4,=0x03004BDC
mov r3,sp
str r3,[r1,0]
str r4,[r1,4]
mov r3,0x81
lsl r3,r3,24
add r3,0x20
str r3,[r1,8]
ldr r3,[r1,8]

; Update pointer to workarea.
str r4,[r7,4]

; Now read each 2 bytes (one row) at a time.  r0 was still charBitmap.
mov r5,r0
mov r6,8

; It's 8 tall, this draws each row to workarea (r4).
drawRow:
ldrh r2,[r5]
strh r2,[r7,0]
add r5,2
; Not sure if something needs this stored already, original code did.
str r5,[r7,12]
;; need to fix regs
bl 0x080710A8
sub r6,1 ; sets Z/eq on 0
bne drawRow

; Since we're done calling funcs, let's restore stack0608.
; This restores charBitmap and puts the old [r7,4] at [r7,12].
mov r6,r10 ; 0x03000604
ldr r3,[r6]
sub r3,2
str r3,[r6]
lsl r2,r3,2
add r2,r9 ; 0x03000608
ldmia r2!,r0,r1
str r0,[r7,8]
str r1,[r7,12]

; Also write 0x20 at [r7,4] for some reason...
mov r2,0x20
str r2,[r7,4]

; At this point, we have the character in the workarea (r4), it's 32 bytes.
; We have at least 64 bytes of space in workarea, which is all we need.

; Before we copy, grab the current x position.
ldr r6,=0x0300198C
ldrb r5,[r6,12]

; And extract which tile to start with (* 4 to get the byte offset at 4bpp.)
mov r2,0xF8
and r2,r5
lsl r2,r2,2

; Load the destination address given that.
ldr r0,[r6,4]
add r2,r2,r0

; Since we're done with xpos, increase it by the charWidth.
ldr r1,=0x08649C4C
; This is the char code stored by 0x0806FA50 when reading the char.
; Previously VWF was disabled because this was statically written as 0xFD.
ldrb r0,[r6,14]
ldrb r1,[r1,r0]
add r0,r5,r1
str r0,[r6,12]

; Now the offset into the tile we should start, as bits (bitStartX.)
mov r3,7
and r3,r5
; Flags from this are used soon.  Sets Z/eq if zero.
lsl r3,r3,2
; Original code stores this multiple times, not sure if actually read.
strb r3,[r7,1]

; At this point, r2=destTile0, r3=bitStartX, r4=workarea, flags=r3==0.  Everything else free.

; If the offset above != 0 (eq/Z flag), jump to where we do the offset.
bne copyOffsetChar

; No offset, we just need to copy the 8 words (8x8 4bpp) over.
ldmia r4!,r3,r5,r6,r7
stmia r2!,r3,r5,r6,r7
ldmia r4!,r3,r5,r6,r7
stmia r2!,r3,r5,r6,r7

; That's it, the tile is ready to go.
b copyDone

; Here's a good place for the literal pool...
.pool

copyOffsetChar:
; Each row is neatly 32 bits, the first pixel is the least significant.
; So we can just shift by the bits calculated above to translate the image.

; Inverse bitStartX for how much we chop off a row to get tile1 data.
mov r5,0x20
sub r7,r5,r3
; This is the BG color, we write this after the character's end.
ldr r6,=0x99999999
lsl r6,r3

; We put 0x20 in r5 earlier, reuse to get to tile1's dest.
add r5,r2
mov r1,8
; Save workarea for tile0.
mov r8,r4

copyOffsetTile1Row:
ldmia r4!,r0
lsr r0,r7
orr r0,r6
stmia r5!,r0
sub r1,1 ; sets Z/eq on 0
bne copyOffsetTile1Row

; Okay, tile1 is done, now we blend tile0.  Reset to workarea start.
mov r4,r8
; r1 has zero, so use that to make 0xFFFFFFFF, which becomes the dst mask.
sub r5,r1,1
lsr r5,r7

mov r1,8

copyOffsetTile0Row:
; Read and translate tile0 content into r0.
ldmia r4!,r0
lsl r0,r3
; Read old content into r7 and blend.
ldr r7,[r2]
; Mask out the bits we're overwriting.
and r7,r5
orr r0,r7
; Now we write the row, done.
stmia r2!, r0
sub r1,1 ; sets Z/eq on 0
bne copyOffsetTile0Row

copyDone:

add sp,4
pop r0-r2,r4-r7
mov r8,r0
mov r9,r1
mov r10,r2
pop r0
bx r0
.pool
.endfunc
.endarea
