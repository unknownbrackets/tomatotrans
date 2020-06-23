; Testing...

.definelabel NameMaxLength,7

; Adjustments for the naming screen entry.
; Allow more chars entered.
; TODO: Do this based on pixel width instead... at best have 5 tiles, some places 4.
.org 0x0802E160
cmp r0,NameMaxLength - 1
.org 0x0802E194
cmp r0,NameMaxLength
; Display up to 7 chars.
.org 0x0802D2FC
mov r0,NameMaxLength
; Check up to 7 chars.
.org 0x0802DD04
mov r0,NameMaxLength

; This function is called on game startup to initialize character data.
; We modify it to allow a longer name.
.org 0x08032C10
.area 0x08032CB8-.,0x00
.func InitCharData
push r4-r6,r14

; Utility params struct.
ldr r4,=0x030018BC
; This is often used to specify indexes (gimmicks, characters, etc.)
ldr r3,=0x03001AD2

; Low nibble is the character to initialize.  High nibble is 8 for non-main char.
ldrb r1,[r4,9]
strb r1,[r3,1]

mov r5,0x0F
and r5,r1
strb r5,[r3,0]

; Okay, now the source.  Stride is just 8.
ldr r6,=DefaultNamesText
lsl r0,r5,3
add r6,r6,r0

; Alright, now calculate the actual length.
str r6,[r4,12]
mov r1,NameMaxLength
strb r1,[r4,1]
bl 0x0806A0D8

; Here's the result length.
ldrb r2,[r4,1]

; Now with r2 set, copy to the char data buffer.
mov r1,r5
mov r0,r6
bl CopyCharNameFromBuffer

; Now init other char data.
bl 0x08032CB8

pop r4-r6,r15
.endfunc

; Args: const char buffer[NameMaxLength], int which, int len
; Returns { char *name (also charInfo), int which, int len }
.func CopyCharNameFromBuffer
; Save len for later.
mov r12,r2
; Alright, calculate the offset into character data (our dest.)
; (x * 2 + 1) * 32 = x * 96
ldr r3,=0x03001EDC
lsl r2,r1,1
add r2,r2,r1
lsl r2,r2,5
add r3,r3,r2

; We can't trust the buffer is aligned... copy byte by byte.
ldrb r2,[r0,0]
strb r2,[r3,0]
ldrb r2,[r0,1]
strb r2,[r3,1]
ldrb r2,[r0,2]
strb r2,[r3,2]
ldrb r2,[r0,3]
strb r2,[r3,3]

; Now any other bytes into free slots in the char struct.
.if NameMaxLength > 4
	ldrb r2,[r0,4]
	; Offset 4 is the length.  Let's keep that for save loading compat.
	strb r2,[r3,5]
.endif
.if NameMaxLength > 5
	ldrb r2,[r0,5]
	strb r2,[r3,6]
.endif
.if NameMaxLength > 6
	ldrb r2,[r0,6]
	mov r0,0x5E
	strb r2,[r3,r0]
.endif
.if NameMaxLength > 7
	.error "Name too long"
.endif

; Okay, we've read the name.  r1 was left unchanged.  Retrieve the len.
mov r2,r12
strb r2,[r3,4]
mov r0,r3

bx r14
.endfunc

; Same as CopyCharName8x8ToVRAM, but with a clear parameter.
; Args: int charNumber, int clearSize
.func CopyCharName8x8ToVRAMClearR1
ldr r2,=MFontClearSize
; Convert to shorts.
lsl r1,r1,4
strh r1,[r2]

.if org() != CopyCharName8x8ToVRAM
	b CopyCharName8x8ToVRAM
.endif
.endfunc

; Essentially CopyString8x8ToVRAM, but sets the string and length.
; Args: int charNumber
.func CopyCharName8x8ToVRAM
push r14

mov r1,r0
ldr r0,=MLongCharNameWorkArea
bl CopyCharNameToBuffer
; This gives us len in r2 and keeps r0/r1 as is.

; Font drawing parameters.
ldr r3,=0x030018BC
strb r2,[r3,5]
str r0,[r3,12]

; The caller fills in the destination address.
bl CopyString8x8ToVRAM

pop r15
.pool
.endfunc
.endarea

; This is how the naming screen preps its buffer.
.org 0x0802D338
.area 0x0802D398-.,0x00
.func NamingScreenLoadName
push r14
; This tells us which name to load.
ldr r1,=0x03001970
ldrb r1,[r1,0]

; This is the struct +0x10, which is the name buffer.
ldr r0,=0x0300199C
bl CopyCharNameToBuffer

; Naming parameters struct.  Set the cursor at the end.
ldr r3,=0x03001AE5
strb r2,[r3,4]

pop r15
.endfunc

; Args: char buffer[NameMaxLength], int which
; Returns { char *buffer, int which, int len }
.func CopyCharNameToBuffer
; Alright, calculate the offset into character data (our dest.)
; (x * 2 + 1) * 32 = x * 96
ldr r3,=0x03001EDC
lsl r2,r1,1
add r2,r2,r1
lsl r2,r2,5
add r3,r3,r2

; We can't trust the buffer is aligned... copy byte by byte.
ldrb r2,[r3,0]
strb r2,[r0,0]
ldrb r2,[r3,1]
strb r2,[r0,1]
ldrb r2,[r3,2]
strb r2,[r0,2]
ldrb r2,[r3,3]
strb r2,[r0,3]

; Now any other bytes into free slots in the char struct.
.if NameMaxLength > 4
	; Offset 4 is the length.  Let's keep that for save loading compat.
	ldrb r2,[r3,5]
	strb r2,[r0,4]
.endif
.if NameMaxLength > 5
	ldrb r2,[r3,6]
	strb r2,[r0,5]
.endif
.if NameMaxLength > 6
	mov r2,0x5E
	ldrb r2,[r3,r2]
	strb r2,[r0,6]
.endif
.if NameMaxLength > 7
	.error "Name too long"
.endif

; Okay, we've read the name.  r0 and r1 were left unchanged.  Grab len in r2.
ldrb r2,[r3,4]

bx r14
.pool
.endfunc
.endarea

; This one is used to save the name from the naming screen.
.org 0x0802D398
.area 0x0802D420-.,0x00
.func NamingScreenSaveName
push r4-r6,r14
; Utility/font params struct.
ldr r4,=0x030018BC
; This is the buffer the current name is stored in.
ldr r5,=0x0300199C

mov r0,0
; This func stores 0 here for some reason, let's keep doing it.
ldr r1,=0x03000600
strh r0,[r1]

; Okay, search for the end.
strb r0,[r4,1]
mov r0,NameMaxLength
strb r0,[r4,4]
str r5,[r4,12]
bl 0x0806A0A0

; Grab the result length - if it's 0xFF, it was all spaces.
ldrb r2,[r4,1]
cmp r2,0xFF
beq @@skipCopy

; This holds the current character num we're naming.
ldr r1,=0x03001970
ldrb r1,[r1,0]

; Okay, we're ready to copy the buffer.
mov r0,r5
bl CopyCharNameFromBuffer

; As a result of that, r0 is now a pointer to the char data.
; Let's store in the params just to follow the original code...
add r0,r0,3
str r0,[r4,12]

; Lastly set the 1 offset to zero, for the same reason.
mov r0,0
strb r0,[r4,1]

@@skipCopy:
pop r4-r6,r15
.pool
.endfunc
.endarea

; This is part of a larger function that handles A buttons on the naming screen.
; Essentially, we want to validate if any other name matches here.
;
; For easy testing, break on 0802D47C, and set 03001970 to 03 there.
.org 0x0802DD76
.area 0x0802DE4E-.,0x00
; At this point, r0=FREE, r1=FREE, r2=0x03000600, r3=0x030018BC
; r4=FREE, r5=0x03001AE5, r6=0, r7=0x03001AE5
; r8=FREE, r9=FREE, r12=FREE, r14=FREE
;
; If we go to the next char (0802DEAC), we must have:
; r4=0x03001AE5, r7=0x030018BC, r8=0x03000600
;
; If there is a matching char name (0802DE4E), we must have:
; r4=0x03001AE5
;
; The original code writes to a lot of temporaries, but they're all overwritten later...

; Let's start by updating things later code we don't patch expects set.
mov r8,r2
mov r7,r3
mov r4,r5

; This is the char info struct array base.
; The current offset (char to check * 0x60) is in 0x030018BC + 4.
ldr r5,[0x0802DE78] ; 0x03001EDC
ldrh r0,[r7,4]
add r5,r5,r0

; And now the current name to compare against.
ldr r6,[0x0802DE84] ; 0x0300199C
; And its current length.
ldrb r3,[r4,4]

; The first four characters are easy.
mov r2,0
@@firstFourLoop:
ldrb r0,[r5,r2]
ldrb r1,[r6,r2]
cmp r0,r1
; Different?  We're done.
bne @@gotoNext

add r2,r2,1
cmp r2,r3
; We checked the length they had, so it's not the same.
; Well, it's the same but shorter.  Confusing, but let's allow it.
beq @@gotoNext
cmp r2,4
blo @@firstFourLoop

; The next two are similar, but offset by one in our character data.
.if NameMaxLength > 4
	ldrb r0,[r5,5]
	ldrb r1,[r6,4]
	cmp r0,r1
	bne @@gotoNext
	cmp r3,5
	beq @@gotoNext
.endif
.if NameMaxLength > 5
	ldrb r0,[r5,6]
	ldrb r1,[r6,5]
	cmp r0,r1
	bne @@gotoNext
	cmp r3,6
	beq @@gotoNext
.endif

; Last one is actually at the end.
.if NameMaxLength > 6
	mov r2,0x5E
	ldrb r0,[r5,r2]
	ldrb r1,[r6,6]
	cmp r0,r1
	bne @@gotoNext
.endif
.if NameMaxLength > 7
	.error "Name too long"
.endif

; Okay, this was a perfect match, show an error message.
b 0x0802DE4E

; We try to jump to this MUCH earlier than the original code, so it's too far away.
@@gotoNext:
b 0x0802DEAC
.endarea
