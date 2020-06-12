; The 8x8 VWF font is already partially implemented in the game.
; The drawing func (080713D0) doesn't blend correctly, so it's not used.
; widthIndex is stored as 0xFD (means normal char) in 0806FA50.
; This patches both of those functions.

@WidthIndex equ 0x08649C4C

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
ldr r0,=0x99999999
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
; Technically that made us unaligned, temporarily.
add sp,4

; Update pointer to workarea.
str r4,[r7,4]

; Now read each 2 bytes (one row) at a time.  r0 was still charBitmap.
mov r5,r0
mov r6,8

; It's 8 tall, this draws each row to workarea (r4).
@@drawRow:
ldrh r2,[r5]
strh r2,[r7,0]
add r5,2
; Not sure if something needs this stored already, original code did.
str r5,[r7,12]
bl 0x080710A8
sub r6,1 ; sets Z/eq on 0
bne @@drawRow

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
ldr r1,=@WidthIndex
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
bne @@copyOffsetChar

; No offset, we just need to copy the 8 words (8x8 4bpp) over.
ldmia r4!,r3,r5,r6,r7
stmia r2!,r3,r5,r6,r7
ldmia r4!,r3,r5,r6,r7
stmia r2!,r3,r5,r6,r7

; That's it, the tile is ready to go.
b @@copyDone

; Here's a good place for the literal pool...
.pool

@@copyOffsetChar:
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

@@copyOffsetTile1Row:
ldmia r4!,r0
lsr r0,r7
orr r0,r6
stmia r5!,r0
sub r1,1 ; sets Z/eq on 0
bne @@copyOffsetTile1Row

; Okay, tile1 is done, now we blend tile0.  Reset to workarea start.
mov r4,r8
; r1 has zero, so use that to make 0xFFFFFFFF, which becomes the dst mask.
sub r5,r1,1
lsr r5,r7

mov r1,8

@@copyOffsetTile0Row:
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
bne @@copyOffsetTile0Row

@@copyDone:

pop r0-r2,r4-r7
mov r8,r0
mov r9,r1
mov r10,r2
pop r0
bx r0
.endfunc

; There's lots of space here, so let's add a new func for width calculation.
; Args: const char *str, uint8_t maxLen, uint8_t ignoreTrailingSpace
; Result struct: r0=str, r1=strLen, r2=pixelWidth, r3=tileWidth
.func Calc8x8PixelWidth
push r4
cmp r2,0
beq @@lengthReady

@@shorter:
; Okay, let's assume it's too long and check.
sub r1,r1,1 ; Sets Z/eq on zero.
beq @@tooShort
ldr r2,[r0,r1]
cmp r2,0
beq @@shorter

; At this point, r1 is one too short.
add r1,r1,1

@@lengthReady:
; Grab the widths
ldr r3,=@WidthIndex
; Save maxLen for later, and start our counter.
mov r12,r1
mov r4,0

@@nextChar:
; Pre-increment so we can ldrb directly.
sub r1,r1,1 ; Sets Z/eq on zero.
beq @@charsDone
ldrb r2,[r0,r1]
; And just grab the width directly.
ldrb r2,[r3,r2]
add r4,r4,r2
b @@nextChar
.pool

@@charsDone:
; Alright, now let's just put the result in the right place.
; r0=str, r1=strLen, r2=pixelWidth, r3=tileWidth
mov r1,r12
mov r2,r4

; And calculate the approx tile width.
add r3,r0,7
lsr r3,r3,3

@@return:
pop r4
bx r14
@@tooShort:
mov r2,0
mov r3,0
b @@return
.endfunc
.endarea


; This is the function that typically draws indexed fixed strings.
; blockSize actually has the full 32 bits in the ROM, which we take advantage of.
.org 0x080748BC
.area 0x080749B8-.,0x00
; Args: uint8_t index, void *dest, uint8_t color, const char *str, uint16_t blockSize
.func Draw8x8FixedStrIndexed
push r4-r7,r14
mov r7,r10
mov r6,r9
mov r5,r8
push r5-r7

; Customization mode: 0=original, 1=maxWidth (buggy), 2=from blockSize
@@mode equ 2

; Truncate r2 (color) and then truncate-and-move index to r4.
mov r4,0xFF
and r2,r4
and r4,r0

; Save dest and str which we'll need later, about to make a call.
mov r10,r1
mov r9,r3
; About to call some funcs, saved regs: (sp+0x20 still has blockSize...)
; r4=index, r5=FREE, r6=FREE, r7=FREE, r8=FREE, r9=str, r10=dest

cmp r2,1
beq @@handleColor1
bgt @@handleColors23
cmp r2,0
beq @@handleColorWhite
b @@colorDone

@@handleColors23:
cmp r2,2
beq @@handleColorGray
cmp r2,3
beq @@handleColorRed
b @@colorDone

@@handleColorWhite:
bl 0x08071A20
b @@colorDone

@@handleColor1:
bl 0x08071A38
b @@colorDone

@@handleColorGray:
bl 0x08071A60
b @@colorDone

@@handleColorRed:
bl 0x08071A4C

@@colorDone:

; Low byte of blockSize is w, next byte is h.  Just read as bytes.
add r5,sp,0x20
ldrb r6,[r5,0]
ldrb r7,[r5,1]
.if @@mode == 2
	ldrb r5,[r5,2]
.endif

; Offset string by w*h*i to get the ith entry.
mul r4,r6
mul r4,r7
add r9,r4
; We only need h for cmp now, so free a low reg.
mov r8,r7

; If height is 0, we're wasting our time.
mov r4,0
cmp r8,r4
beq @@skipDrawing

.if @@mode == 1
	; Track what this mode needs: max xpos (actually bytes.)
	mov r5,0
.endif
; Font drawing parameters are set here.
ldr r7,=0x030018BC

@@drawNextLine:
strb r6,[r7,5]
mov r2,r6
mul r2,r4
; Offset workarea by y * w * 32 bytes.
ldr r1,=0x030041DC
lsl r3,r2,5
add r3,r1
str r3,[r7,8]
add r2,r9
str r2,[r7,12]
; All set, now we draw the string to workarea.
bl 0x08071748

.if @@mode == 1
	;; Grab the bytes written (accounting for VWF.)
	ldr r0,[r7,4]
	cmp r0,r5
	blo @@skipXUpdate
	mov r5,r0
	@@skipXUpdate:
.endif

add r4,r4,1
cmp r4,r8
blo @@drawNextLine

; At this point, we've drawn the index-offset lines into workarea,
; and we have the max line width.  Now for the DMA3 transfer.

; Prepare the DMA3 transfer params for 32-bit.
ldr r1,=0x040000D4
.if @@mode == 0
	; Original mode, use max length.
	lsl r5,r6,3
.elseif @@mode == 1
	; Convert maxXPos from bytes to words.  This is the amount for each copy.
	lsr r5,r5,3
.elseif @@mode == 2
	; In this mode, when r5 is not 0, override the max length.
	lsl r5,r5,3 ; Sets Z=1 if zero.
	bne @@haveOverride
	lsl r5,r6,3
	@@haveOverride:
.endif
mov r2,0x84
lsl r2,r2,24
orr r2,r5
ldr r3,=0x030041DC

; Trade width for the byte offset so we don't recalc each line.
lsl r6,r6,5

mov r4,0
@@copyNextLine:
str r3,[r1,0]
add r3,r6
; Each line is offset by 0x400.
lsl r0,r4,10
add r0,r10
str r0,[r1,4]
str r2,[r1,8]
ldr r0,[r1,8]

add r4,r4,1
cmp r4,r8
blo @@copyNextLine

@@skipDrawing:
; Restore color to white.
bl 0x08071A20

pop r5-r7
mov r10,r7
mov r9,r6
mov r8,r5
pop r4-r7
pop r3
bx r3
.pool
.endfunc
.endarea

; We modify this func to adjust the start xpos on misaligned tiles.
; There's no reason to allow text rotating wrong on tiles, so we treat that as an x offset for VWF.
org 0x08071748
.area 0x0807184C-.,0x00
.func CopyString8x8ToVRAM
push r4-r7,r14
ldr r0,=0x99999999
push r0

; Draw char parameters struct.
ldr r2,=0x0300198C
; Drawing parameters struct.
ldr r4,=0x030018BC
mov r1,0
; Not sure if this is ypos or something?  12 is xpos.
strb r1,[r2,13]

; Load the string length.
ldrb r7,[r4,5]

; Grab the destination address into r3.
ldr r3,[r4,8]
; Grab the top nibble - 6 means vram, so we apply our offset.
lsr r0,r3,24
cmp r0,6
bne @@normalAlign

; Down align to the tile.
mov r1,31
and r1,r3
bic r3,r1
; 8 pixels per tile, so divide 31 by 4.
lsr r1,r1,2

; Now let's figure the next Y position so we don't overflow our clear.
lsr r5,r3,10
add r5,r5,1
lsl r5,r5,10
; This is the max bytes to clear now, convert to shorts.
sub r5,r3
lsr r5,r5,1
; And what we intend to clear, in shorts.
lsl r6,r7,4

cmp r6,r5
bls @@alignDone
mov r6,r5
b @@alignDone

@@normalAlign:
; x = (x + 3) & ~3;
mov r0,3
add r3,3
bic r3,r0
; r1 is still 0 from above, that will be x also.

; Since we're clearing, let's just assume we have at most 0x1A tiles of space.
; The VWF makes longer strings take less than 32 bytes each char, so this may still be too much.
mov r5,0x1C
cmp r7,r5
bhi @@cappedWidth
mov r5,r7
@@cappedWidth:
lsl r6,r5,4

@@alignDone:
; Now store that as the starting xpos and store the dest.
strb r1,[r2,12]
str r3,[r2,4]

; Okay, use DMA3 to clear those bytes.
ldr r0,=0x040000D4
mov r1,sp
str r1,[r0,0]
str r3,[r0,4]
mov r1,0x81
lsl r1,r1,24
; We put the shorts to clear in r6.
orr r1,r6
str r1,[r0,8]
ldr r1,[r0,8]

ldr r5,=0x03000604
ldr r6,=0x03000608

@@nextChar:
; The old code saves/restores len in 03000608, we just keep it in r7.
; Nothing we call reads it from there.

; This grabs the next character and bitmap, accounting for FF codes.
bl 0x0806FA50
ldrb r0,[r4,1]

; If we hit 0, 1, 3, 4 (END, WAITBREAK, PAUSEBREAK, CLEAR), stop.
cmp r0,2
beq @@skipCharDraw
cmp r0,4
bls @@done

; We only draw if it was > 0xFB (primarily 0xFD...)
cmp r0,0xFB
bls @@skipCharDraw

; Save the current string pointer, because this func overwrites it.
ldr r0,[r5]
lsl r1,r0,2
ldr r2,[r4,12]
str r2,[r6,r1]
add r0,r0,1
str r0,[r5]

; Draw the character
bl CopyChar8x8ToVRAM

; Restore that string pointer.
ldr r0,[r5]
sub r0,r0,1
lsl r1,r0,2
ldr r2,[r6,r1]
str r2,[r4,12]
str r0,[r5]

@@skipCharDraw:
; One less char left.  We mask 0xFF in case it started with 0 and becomes -1.
; That'd be a bug anyway, but I'd rather not break worse than the original code.
sub r7,r7,1
lsl r0,r7,24
lsr r0,r0,24

; This is a straight bx r14, but let's call it anyway.
bl 0x0802FEF0

ldr r1,=0x03000600
strh r0,[r1]

cmp r0,0
bne @@nextChar

@@done:
; Grab the final x position.
ldr r2,=0x0300198C
ldrb r0,[r2,12]

; Store directly in fontInfo's 1 offset, and times 8 (pixels?) in 4.
; Note that the word at 4 is twice the number of bytes.
strb r0,[r4,1]
lsl r0,r0,3
str r0,[r4,4]

add sp,4
pop r4-r7
pop r0
bx r0
.pool
.endfunc
.endarea
