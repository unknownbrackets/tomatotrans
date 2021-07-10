; When using [CENTER], the width is calculated assuming all letters are 8 long.
; This changes the related functions to account for the VWF.

; This is the 8x12 width index.
@WidthIndex equ 0x08649D4D

.org 0x0806E7EC
.region 0x0806E80C-.,0x00
; We change this to simply take the new length in r0, simpler to use.
; Args: uint8_t addLength, struct *utilityParams
; Returns: uint8_t newLength, struct *utilityParams
.func CalcLengthAppendWidth
; r1 should be the utility params struct - offset 5 is where the width so far is.
ldrb r2,[r1,5]
add r0,r0,r2

cmp r0,0x90
blt @@skipCap
mov r0,0x90
@@skipCap:

strb r0,[r1,5]
bx r14
.endfunc
.endregion

.org 0x0806E80C
.region 0x0806EC90-.,0x00
.func CalcLengthDialog
push r4,r14
; We track the widths in this struct.
ldr r1,=0x030018BC
; Initialize width to zero.
mov r0,0
strb r0,[r1,5]

; This struct has flags for wide and tall, we only care about wide.
ldr r2,=0x03004188 + 0x23
ldrb r0,[r2,0]
; Start with the standard size.
mov r2,8

; Double r2 if we're set as wide.
mov r3,1
and r0,r3
lsl r2,r0

; Originally, 8=kanji, 9=regular.  We put the shift size in 8 instead.
strb r0,[r1,8]
strb r2,[r1,9]

; Now load the string pointer.
ldr r4,[r1,12]

@@nextChar:
ldrb r0,[r4,0]
add r4,r4,1

cmp r0,0xFE
beq @@kanjiChar
bhi @@controlCode

; Okay, just a regular character.  The common case, probably?
ldr r3,=@WidthIndex
add r0,r0,r3
ldrb r0,[r0,0]
ldrb r2,[r1,8]
lsl r0,r2
bl CalcLengthAppendWidth
b @@nextChar
.pool

@@kanjiChar:
; These are always 12 pixels (or double) wide.
ldrb r2,[r1,8]
mov r0,12
lsl r0,r2
bl CalcLengthAppendWidth
add r4,r4,1
b @@nextChar

@@controlCode:
ldrb r0,[r4,0]
add r4,r4,1

; END, WAIT, CONTINUE, PAUSEBREAK, CLEAR, and BREAK all mean we're done.
cmp r0,5
bls @@widthDone

cmp r0,0x80
bhs @@code8x

; Bugfix: keep centering for SIMPLEWAIT.
cmp r0,0x40
bhi @@widthDone

; If lower, it'll match the next hi check.
sub r0,0x10
beq @@ignoreParam

cmp r0,0x27 - 0x10
beq @@code27Time
bhi @@nextChar

; At this point it must be between 0x11 (as 0x01) and 0x26 (as 0x16.)
; The rest we'll sort out using a lookup table - off by one because it starts at 0x11.
add r3,=@@code11To26Table-4
lsl r2,r0,2
ldr r2,[r3,r2]
mov r15,r2

@@code11To26Table:
dw @@ignoreParam ; 0x11 [PAUSE XX]
dw @@nextChar ; 0x12 [SETPAUSE]
dw @@nextChar ; 0x13
dw @@nextChar ; 0x14
dw @@nextChar ; 0x15
dw @@nextChar ; 0x16
dw @@nextChar ; 0x17
dw @@ignoreParam ; 0x18 [SET_X XX]
dw @@ignoreParam ; 0x19 [SET_Y XX]
dw @@ignoreParam ; 0x1A [?? XX]
dw @@nextChar ; 0x1B
dw @@code1CESizeWide ; 0x1C [SIZE_WIDE]
dw @@code1DFSizeNormal ; 0x1D [SIZE_TALL]
dw @@code1CESizeWide ; 0x1E [SIZE_BIG]
dw @@code1DFSizeNormal ; 0x1F [SIZE_NORMAL]
dw @@nextChar ; 0x20
dw @@nextChar ; 0x21
dw @@ignoreParam ; 0x22 [?? XX]
dw @@ignoreParam ; 0x23 [?? XX]
dw @@nextChar ; 0x24 [CENTER_H]
dw @@nextChar ; 0x25 [CENTER_V]
dw @@nextChar ; 0x26 [CENTER_HV]

@@ignoreParam:
add r4,r4,1
b @@nextChar

@@code1CESizeWide:
mov r3,1
mov r2,16
strb r3,[r1,8]
strb r2,[r1,9]
b @@nextChar

@@code1DFSizeNormal:
mov r3,0
mov r2,8
strb r3,[r1,8]
strb r2,[r1,9]
b @@nextChar

@@code27Time:
; [TIME] ends up like 00:00:00.  Start with the colon.
ldr r3,=@WidthIndex
mov r2,0x7E ; ':'
ldrb r0,[r3,r2]
; Double that if needed.
ldrb r2,[r1,8]
lsl r0,r2

; Now we add the digits, always standard width.  Start with three of them.
ldrb r2,[r1,9]
add r3,r2,r2
add r3,r3,r2
; Add to the colon width.
add r0,r0,r3

; That's the width of '00:0'.  Double it to get the rest.
add r0,r0,r0
bl CalcLengthAppendWidth
b @@nextChar

@@code8x:
; Flags still set from cmp to 0x80.
beq @@code80Number

cmp r0,0x85
bhi @@nextChar
beq @@code85Item

; Okay, it's a name from 0x81 - 0x84.
sub r0,0x81
bl CalcLengthDialog8xName
ldr r1,=0x030018BC
b @@nextChar

@@code80Number:
bl CalcLengthDialog80Number
ldr r1,=0x030018BC
b @@nextChar

@@code85Item:
bl CalcLengthDialog85Item
ldr r1,=0x030018BC
b @@nextChar

@@widthDone:
; We still have the utility params in r1.  Calculate the X offset.
ldrb r0,[r1,5]
mov r3,0x90
sub r3,r3,r0
lsr r3,r3,1

ldr r2,=0x03004188
strb r3,[r1,1]
strb r3,[r2,11]

pop r4
pop r0
bx r0
.endfunc

; This function calculates the length of [NUMBER].
; It was fine before, but we rewrite to make space for other funcs.
.func CalcLengthDialog80Number
; This is the number to display.
ldr r0,=0x03003A24
; Was originally signed, but was totally broken for negative anyway.
ldrh r0,[r0,0]

; Next load the x pos and then width of one number.
ldr r3,=0x030018BC
ldrb r1,[r3,5]
ldrb r2,[r3,9]

cmp r0,9
ble @@singleDigit
cmp r0,99
ble @@twoDigits

@@moreDigits:
; Divide by 8.  Now we compare against 125 for four digits plus.
asr r0,r0,3
cmp r0,1000 / 8
blt @@threeDigits
; Sub 250 (2000 from orig), then divide again so we can compare with 125 for 9999.
; This is still cheaper than the bytes for a literal + its load.
; 125 * 64 + 2000 = 10,000 (and it keeps going like that.)
; This is safe as long as we compare signed.
add r1,r2
sub r0,250
bgt @@moreDigits

@@threeDigits:
add r1,r2
@@twoDigits:
add r1,r2
@@singleDigit:
add r1,r2

; Alright, now r1 is the total width.  Cap at 0x90.
cmp r1,0x90
blt @@belowCap
mov r1,0x90
@@belowCap:
strb r1,[r3,5]
bx r14
.endfunc

; Args: const char *str, uint8_t maxLen
; Result struct: r0=str, r1=strLen, r2=pixelWidth
.func Calc8x12PixelWidth
push r4
; Grab the 8x12 widths.
ldr r3,=@WidthIndex
; Save maxLen for later, and start our counter.
mov r12,r1
mov r4,0

@@nextChar:
; Pre-decrement so we can ldrb directly.  Doesn't matter if we add widths in reverse.
sub r1,r1,1 ; Sets N/mi on negative, so we do the last one.
bmi @@charsDone
ldrb r2,[r0,r1]
; And just grab the width directly.
ldrb r2,[r3,r2]
add r4,r4,r2
b @@nextChar
.pool

@@charsDone:
; Alright, now let's just put the result in the right place.
; r0=str, r1=strLen, r2=pixelWidth
mov r1,r12
mov r2,r4

@@return:
pop r4
bx r14
.endfunc

.func CalcLengthDialog8xName
push r14

; The character number is in r0.
.ifdef NameMaxLength
	mov r1,r0
	ldr r0,=MLongCharNameWorkArea
	bl CopyCharNameToBuffer
	mov r1,r2
.else
	; Multiply r0 by 96.
	lsl r1,r0,1
	add r1,r1,r0
	lsl r0,r1,5

	ldr r1,=0x03001EDC
	add r0,r0,r1
	ldrb r1,[r0,4]
.endif

mov r2,0
bl Calc8x12PixelWidth
; r2 is now the pixel width.

ldr r3,=0x030018BC
; Get the width multiplier (this is a shift amount.)
ldrb r0,[r3,8]
lsl r2,r0

; Okay, get the x pos and add it.
ldrb r0,[r3,5]
add r0,r0,r2
cmp r0,0x90
blt @@belowCap
mov r0,0x90
@@belowCap:
strb r0,[r3,5]

pop r15
.pool
.endfunc
.endregion
