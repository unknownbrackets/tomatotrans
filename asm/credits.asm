; Move the crown to the right to give space for Difficulty.
.org 0x0802BB10
dh 0xF474
.org 0x0802BB90
dh 0xF474

; The original game stores the credits in a sort of modified Shift-JIS format.
; It then decodes this and uses bytecodes to indicate hiragana/katakana.
; We replace that by just using the game's text encoding directly, which is simpler.
; This also allows any characters available in the game's text encoding.

.org 0x0802B08C
.region 0x0802B244-.,0x00
.func CreditsDrawNextLine
push r4-r7,r14
; A few credits flags are offset from here.
ldr r7,=0x03004EDC

; At ,1 is the target speed, and ,2 current counter.
ldrb r0,[r7,6]
add r0,r0,1
strb r0,[r7,6]
ldrb r1,[r7,5]

; Did we hit it yet?
cmp r0,r1
bne @@return

; We did, reset back to zero for next time.
mov r0,0
strb r0,[r7,6]

; Now the y offset is here.  We increment it too.
ldr r2,=0x0300149E
ldrh r1,[r2,2]
add r1,r1,1
strh r1,[r2,2]

; We activate every 8 pixels (1 tile) - are we there yet?
mov r4,7
and r4,r1 ; Sets Z/eq if zero.
bne @@return

; Freebie: we track size=0 (8x12) or 1 (8x8) in r4, and it's already 0 now.

; Clear ahead of where we're going to draw (not sure exactly why.)
; Start by putting a 0 into memory as the source of the clear.
add sp,-4
str r4,[sp,0]
add r2,sp,0
add sp,4
; Okay now let's prepare the destination address from the y offset (in r1.)
mov r6,0xF0
add r6,r6,r1
mov r5,0xF8
and r6,r5
lsl r6,r6,7
; Add in the VRAM base, which we'll reuse later.
mov r5,0x06
lsl r5,r5,24
add r6,r6,r5

; Okay, time to fill in the DMA3 transfer.
ldr r3,=0x040000D4
str r2,[r3,0]
str r6,[r3,4]
ldr r2,=0x850000A0 ; Copy 0x0280 zeros.
str r2,[r3,8]

; Okay, now time to prepare our drawing destination into r6.
mov r6,0xA0
add r6,r6,r1
mov r2,0xF8
and r6,r2
lsl r6,r6,7
; And add 3 tiles x offset too.
add r6,0x60
; We saved the VRAM base here, time for that reuse.
add r6,r6,r5

; Okay, now let's get the credits bytecode address into r5.
ldr r5,[r7,0]

@@nextBytecode:
ldrb r0,[r5,0]
add r5,r5,1

; If it's above 0xFE (END), it can only be 0xFF (LINE BREAK).
cmp r0,0xFE
bhi @@codeBreak
beq @@codeEnd

; Now check for 0xFD (SMALL) and 0xFB (SIGNAL) the same way.
; We removed 0xFC, so it's treated as 0xFD.
cmp r0,0xFB
bhi @@codeSmall
beq @@codeSignal

; Last special code, 0xFA means to DMA3 a graphic instead.
cmp r0,0xFA
beq @@codeIcon

; Okay, we're drawing a string (r0=length), get the params struct.
ldr r3,=0x030018BC
strb r0,[r3,5]
str r6,[r3,8]
str r5,[r3,12]
; And advance the string by that length.
add r5,r5,r0

cmp r4,1
beq @@line8x8

bl CopyString8x12ToVRAM
b @@nextBytecode

@@line8x8:
bl CopyString8x8ToVRAM
b @@nextBytecode

@@codeIcon:
; Read which icon.
ldrb r1,[r5,0]
add r5,r5,1

; Calculate the offset into the data.
; r1 * 0x320 = ((r1 * 2 + r1) * 8 + r1) * 0x20
lsl r0,r1,1
add r0,r0,r1
lsl r0,r0,3
add r0,r0,r1
lsl r0,r0,5

ldr r1,=0x08480DA4
add r0,r0,r1

; Okay, now just copy using a DMA3.
ldr r3,=0x040000D4
str r0,[r3,0]
str r6,[r3,4]
ldr r1,=0x840000C8 ; Copy 0x0320 bytes.
str r1,[r3,8]
b @@nextBytecode

@@codeSmall:
mov r4,1
b @@nextBytecode

@@codeSignal:
mov r0,1
strb r0,[r7,17] ; 0x03004EED
b @@nextBytecode

@@codeEnd:
; Store flags indicating end.
mov r0,2
mov r1,0x78
strb r0,[r7,17] ; 0x03004EED
strh r1,[r7,20] ; 0x03004EF0
b @@return

@@codeBreak:
; Store the bytecode pointer back.
str r5,[r7,0]
; Fall through to return.

@@return:
pop r4-r7,r15
.pool
.endfunc
.endregion

.org 0x0802B43C
; We delete the single char 8x12 and 8x8 funcs, and the shift-jis decoder.
; In their place, we add a full string 8x12 menu text drawing func.
.region 0x0802B7F0-.,0x00
.func CopyString8x12ToVRAM
push r4-r6,r14
; Write to a temporary, since it's top/bottom tiles.
ldr r5,=0x030041DC
ldr r6,=0x030018BC
; Save the actual dest address.
ldr r4,[r6,8]
str r5,[r6,8]

; Call the actual (out of order) 8x12 drawing func.
bl 0x08071624
; Grab the width in pixels.
ldrb r6,[r6,1]

; Now prepare the next row of dest into r1, which might wrap around.
; Start with the base into r1.
ldr r0,=0x7FFF
mov r1,r4
bic r1,r0
; Now the offset of 0x400, which is to advance the Y position.
mov r2,0x40
lsl r2,r2,4
; Add to dest and mask to wrap.
add r2,r2,r4
and r0,r2
; Now we just combine them back together.
orr r1,r0

; Prepare the DMA3 address.
ldr r3,=0x040000D4
; Transfer size and activate flag (we always copy 32 bytes as 16-bit halfwords.)
mov r2,0x80
lsl r2,r2,24
add r2,0x10

; Each character is two tiles.  We unswizzle them here.
@@nextTilePair:
// Execute a DMA3 copy from src -> dest.
str r5,[r3,0]
str r4,[r3,4]
str r2,[r3,8]

; Add 0x20 to src, and 0x20 to dest.
add r5,0x20
add r4,0x20

// Execute a DMA3 copy from src -> dest.
str r5,[r3,0]
str r1,[r3,4]
str r2,[r3,8]

; Another 0x20 for src, but 0x20 to dest2 not dest.
add r5,0x20
add r1,0x20

sub r6,8 ; Sets flags if positive still
bgt @@nextTilePair

pop r4-r6,r15
.pool
.endfunc
.endregion

; So that the 8x12 font actually uses VWF correctly, we update the lookup func to load the width index.
; It just leaves it alone, so it's typically 0 and the width is wrong.
; The original code hacked around this bug by drawing a string of length 1 at a time.
.org 0x0806F9C4
.region 0x0806FA50-.,0x00
.func GetNext8x12Char
; Font/utility params struct.
ldr r3,=0x030018BC
; Save out the previous value here.
ldrb r0,[r3,1]
ldr r2,=0x03001AD2
strb r0,[r2,0]

; Now read in the current string pointer.
ldr r2,[r3,12]

; Read the byte or control code and store in util.
ldrb r0,[r2]
add r2,r2,1
strb r0,[r3,1]

; If it's above FE (KANJI), it can only be FF (CONTROL CODE.)
cmp r0,0xFE
beq @@codeKanji
bhi @@codeControl

; Regular characters are FD or 00-FB, not FC (not sure why.)
cmp r0,0xFC
beq @@done

; This is the important bit: store the character so we look up its width later.
ldr r1,=0x0300198C
strb r0,[r1,14]
; Also store as the arg, not sure anything reads this...
strb r0,[r3,5]

; Write 0xFD as the control code.
mov r1,0xFD
strb r1,[r3,1]

; Now get the glyph for this character, 32 bytes per.
ldr r1,=0x08642748
lsl r0,r0,5
add r0,r0,r1
; And store in util as dest.
str r0,[r3,8]
b @@done

@@codeControl:
; Okay, overwrite with the read control code.
ldrb r0,[r2]
add r2,r2,1
strb r0,[r3,1]

cmp r0,0x11
beq @@readControlArg
cmp r0,0x10
beq @@readControlArg
cmp r0,0x03
beq @@readControlArg
b @@done

@@readControlArg:
ldrb r0,[r2]
add r2,r2,1
strb r0,[r3,5]
b @@done

@@codeKanji:
; This we read and store into the arg (offset 5.)
ldrb r0,[r2]
add r2,r2,1
strb r0,[r3,5]

; And now get the bitmap into the util as dest (64 bytes per.)
ldr r1,=0x08644748
lsl r0,r0,6
add r0,r0,r1
str r0,[r3,8]
; Fall through to @@done.

@@done:
; Write the updated string pointer.
str r2,[r3,12]

bx r14
.pool
.endfunc
.endregion

; We also edit the 8x12 out of order drawing func to clear properly.
.org 0x08071650
; Need to clear 64 bytes per character, not 32 (this is a count of shorts.)
lsl r0,r0,5
