@ButtonText equ NamingScreenText
@ButtonLength equ 8
@MessageText equ NamingScreenText + @ButtonLength * 4

; Force English keyboad format on Left/Right/Up/Down.
; Also prevent access to the prev Katakana/Hiragana switch.
.org 0x0802D6A0
nop
.org 0x0802D6A6
sub r0,r0,3
.org 0x0802D7F6
nop
.org 0x0802D7FC
sub r0,r0,3
.org 0x0802D94A
nop
.org 0x0802D9C2
sub r0,r0,3
.org 0x0802DABC
nop
.org 0x0802DB32
sub r0,r0,3
.org 0x0802DB3A
mov r0,3

; Only allow switching between two keyboards.
.org 0x0802D5CA
cmp r0,2
; Change the remaining switch to 0/1.
.org 0x0802DC54
sub r0,r0,1
.org 0x0802DC5C
mov r0,0
.org 0x0802DC64
mov r0,1

; No need to reset X/Y when switching keyboards anymore.
.org 0x0802D5BC
nop
nop

; Now make the Katakana keyboard generate uppercase.
.org 0x0862C0D4
db 0x01,0x02,0x03,0x04,0x05
db 0x06,0x07,0x08,0x09,0x0A
db 0x0B,0x0C,0x0D,0x0E,0x0F
db 0x10,0x11,0x12,0x13,0x14
db 0x15,0x16,0x17,0x18,0x19
db 0x1A,0x00,0x00,0x00,0x00
db 0x71,0x72,0x73,0x74,0x75
db 0x76,0x77,0x78,0x79,0x70
db 0x7A,0x7B,0x7C,0x7D,0x7E
db 0x7F,0x00,0x00,0x00,0x00
db 0x00,0x00,0x00,0x00,0x00
db 0x00,0x00,0x00,0x00,0x00
; And lowercase for the old Hiragana keyboard.
.org 0x0862C110
db 0x81,0x82,0x83,0x84,0x85
db 0x86,0x87,0x88,0x89,0x8A
db 0x8B,0x8C,0x8D,0x8E,0x8F
db 0x90,0x91,0x92,0x93,0x94
db 0x95,0x96,0x97,0x98,0x99
db 0x9A,0x00,0x00,0x00,0x00
db 0x71,0x72,0x73,0x74,0x75
db 0x76,0x77,0x78,0x79,0x70
db 0x7A,0x7B,0x7C,0x7D,0x7E
db 0x7F,0x00,0x00,0x00,0x00
db 0x00,0x00,0x00,0x00,0x00
db 0x00,0x00,0x00,0x00,0x00

; Now to replace the old Katakana graphics.  30 per row.
.org 0x0863A782
dh 0xF041, 0xF042, 0xF043, 0xF044, 0xF045
dh 0xF005
dh 0xF0B1, 0xF0B2, 0xF0B3, 0xF0B4, 0xF0B5
.org 0x0863A782 + 30 * 2 * 2
dh 0xF046, 0xF047, 0xF048, 0xF049, 0xF04A
dh 0xF005
dh 0xF0B6, 0xF0B7, 0xF0B8, 0xF0B9, 0xF0B0
.org 0x0863A782 + 30 * 2 * 4
dh 0xF04B, 0xF04C, 0xF04D, 0xF04E, 0xF04F
dh 0xF005
dh 0xF0BA, 0xF0BB, 0xF0BC, 0xF0BD, 0xF0BE
.org 0x0863A782 + 30 * 2 * 6
dh 0xF050, 0xF051, 0xF052, 0xF053, 0xF054
dh 0xF005
dh 0xF0BF, 0xF05E, 0xF005, 0xF005, 0xF005
.org 0x0863A782 + 30 * 2 * 8
dh 0xF055, 0xF056, 0xF057, 0xF058, 0xF059
dh 0xF005
dh 0xF005, 0xF005, 0xF005, 0xF005, 0xF005
.org 0x0863A782 + 30 * 2 * 10
dh 0xF05A, 0xF005, 0xF005, 0xF005, 0xF005
dh 0xF005
dh 0xF005, 0xF005, 0xF005, 0xF005, 0xF005
; And get rid of the symbols on the 12th column.
.org 0x0863A782 + 12 * 2
dh 0xF005, 0xF005, 0xF005, 0xF005
.org 0x0863A782 + 30 * 2 * 2 + 12 * 2
dh 0xF005, 0xF005, 0xF005, 0xF005
; And the old Kata/Hira switch.
.org 0x0863A782 + 30 * 2 * 4 + 12 * 2
dh 0xF005, 0xF005, 0xF005, 0xF005
.ifdef NameMaxLength
	; Show more tiles for the name.
	.org 0x863A6D8
	dh 0xF140, 0xF141, 0xF142, 0xF143, 0xF144, 0xF145 ; 0x06002800
	; For the keyboard, default, and accept buttons - let's give ourselves 5 tiles.
	.org 0x0863A782 + 30 * 2 * 6 + 12 * 2
	dh 0xF158, 0xF159, 0xF15A, 0xF15B, 0xF15C ; 0x06002B00
	.org 0x0863A782 + 30 * 2 * 8 + 12 * 2
	dh 0xF15D, 0xF15E, 0xF15F, 0xF160, 0xF161 ; 0x06002BA0
	.org 0x0863A782 + 30 * 2 * 10 + 12 * 2
	dh 0xF162, 0xF163, 0xF164, 0xF165, 0xF166 ; 0x06002C40
	; Move the message a bit to give more space for the name.
	.org 0x0863AA8C
	; 0x060028C0 (was 0x06002880)
	dh 0xF146, 0xF147, 0xF148, 0xF149, 0xF14A, 0xF14B, 0xF14C, 0xF14D, 0xF14E, 0xF14F, 0xF150, 0xF151, 0xF152, 0xF153, 0xF154, 0xF155, 0xF156, 0xF157
.else
	; For the keyboard, default, and accept buttons - let's give ourselves 5 tiles.
	.org 0x0863A782 + 30 * 2 * 6 + 12 * 2
	dh 0xF156, 0xF157, 0xF158, 0xF159, 0xF15A ; 0x06002AC0
	.org 0x0863A782 + 30 * 2 * 8 + 12 * 2
	dh 0xF15B, 0xF15C, 0xF15D, 0xF15E, 0xF15F ; 0x06002B60
	.org 0x0863A782 + 30 * 2 * 10 + 12 * 2
	dh 0xF160, 0xF161, 0xF162, 0xF163, 0xF164 ; 0x06002C00
.endif

; Next up, the old Hiragana graphics - lowercase.
; Note: this is just the update area, 11 per row.
.org 0x0863AAFA
dh 0xF0C1, 0xF0C2, 0xF0C3, 0xF0C4, 0xF0C5
dh 0xF005
dh 0xF0B1, 0xF0B2, 0xF0B3, 0xF0B4, 0xF0B5
.org 0x0863AAFA + 11 * 2 * 2
dh 0xF0C6, 0xF0C7, 0xF0C8, 0xF0C9, 0xF0CA
dh 0xF005
dh 0xF0B6, 0xF0B7, 0xF0B8, 0xF0B9, 0xF0B0
.org 0x0863AAFA + 11 * 2 * 4
dh 0xF0CB, 0xF0CC, 0xF0CD, 0xF0CE, 0xF0CF
dh 0xF005
dh 0xF0BA, 0xF0BB, 0xF0BC, 0xF0BD, 0xF0BE
.org 0x0863AAFA + 11 * 2 * 6
dh 0xF0D0, 0xF0D1, 0xF0D2, 0xF0D3, 0xF0D4
dh 0xF005
dh 0xF0BF, 0xF05E, 0xF005, 0xF005, 0xF005
.org 0x0863AAFA + 11 * 2 * 8
dh 0xF0D5, 0xF0D6, 0xF0D7, 0xF0D8, 0xF0D9
dh 0xF005
dh 0xF005, 0xF005, 0xF005, 0xF005, 0xF005
.org 0x0863AAFA + 11 * 2 * 10
dh 0xF0DA, 0xF005, 0xF005, 0xF005, 0xF005
dh 0xF005
dh 0xF005, 0xF005, 0xF005, 0xF005, 0xF005

; 0802D12C calls two functions next to each other to update buttons.
; Let's just update them all at once in one function.
.org 0x0802D138
nop
nop

; Need to update the message when English is selected.
.org 0x0802E4EE
sub r0,r0,1
; And always show the space on our space.
.org 0x0802E548
b 0x802E54E

; And also need to adjust the arrow position.
.org 0x0802E794
mov r0,0xA6
.org 0x0802E780
mov r0,0x85

; This corrects a bug in the game in checking for name reuse.  Oops.
.ifndef NameMaxLength
.org 0x0802DDBA
	ldrb r0,[r7,8]
.endif

; This draws the buttons to the right of the letters.
.org 0x0802D220
.area 0x0802D2F0-.,0x00
.func NamingScreenUpdateButtons
push r4-r6,r14
; Naming screen parameters.
ldr r5,=0x3001AE5
ldr r4,=@ButtonText
mov r1,r4

; What keyboard are we showing?
ldrb r0,[r5,3]
cmp r0,0
beq @@lowerButton
add r1,@ButtonLength
@@lowerButton:

; We don't need the screen params anymore, trade for the text draw param struct.
ldr r5,=0x030018BC
mov r0,@ButtonLength
strb r0,[r5,5]
; Temporary buffer for drawing.
ldr r6,=0x030041DC
str r6,[r5,8]
str r1,[r5,12]
mov r0,5
bl CopyString8x8ClearR0

; Okay, next up Default.  Move the buffer and text pos.
add r4,@ButtonLength * 2
add r6,0xA0
mov r0,@ButtonLength
strb r0,[r5,5]
str r6,[r5,8]
str r4,[r5,12]
mov r0,5
bl CopyString8x8ClearR0

; We still have space in our buffer, last button.
add r4,@ButtonLength
add r6,0xA0
mov r0,@ButtonLength
strb r0,[r5,5]
str r6,[r5,8]
str r4,[r5,12]
mov r0,5
bl CopyString8x8ClearR0

; Drawing done, now let's get the base buffer pos back.
sub r6,0xA0
sub r6,0xA0

; Setup the copy to VRAM.  First multiply 0xA0 by 3.
mov r0,0xA0
lsl r1,r0,1
add r0,r0,r1
strh r0,[r5,4]
str r6,[r5,8]
ldr r0,=0x04000008
ldrh r1,[r0]
; Check the correct BG charblock.
mov r0,0x000C
and r0,r1
lsl r0,r0,12
.ifdef NameMaxLength
	ldr r1,=0x06002B00
.else
	ldr r1,=0x06002AC0
.endif
add r0,r1
str r0,[r5,12]
bl 0x0806B514

pop r4-r6,r15
.endfunc

.ifdef NameMaxLength
; No args, same as below.
.func NamingScreenCanAddCurrent
; Name params struct.
ldr r3,=0x03001AE5
ldrb r0,[r3,5]
; Intentionally continues to NamingScreenCanAddLetter.
.endfunc
; Args: uint8_t letterIndex
; Returns 1 if that letter can fit
.func NamingScreenCanAddLetter
push r4,r14
; Start by getting the letter.
bl NamingScreenGetLetter

; How wide is that?
ldr r1,=0x08649C4C
ldrb r4,[r1,r0]

; Get the current length from the naming params
ldr r3,=0x03001AE5
ldrb r1,[r3,4]
cmp r1,NameMaxLength
bhs @@nope

; This buffer has the string so far.
ldr r0,=0x0300199C
mov r2,0
bl Calc8x8PixelWidth

; Okay r2 is the current pixel width, add our new letter.
add r1,r2,r4
mov r0,1

; We allow up to 5 tiles.
cmp r1,NameMaxTiles * 8
bls @@return

@@nope:
; Not enough space, sorry.
mov r0,0

@@return:
pop r4,r15
.endfunc

; Returns 1 if more letters can fit.
.func NamingScreenCanAddMore
push r14
; Let's cheat and just ask if we can add an 'i'.
mov r0,0x89
bl NamingScreenCanAddLetter
pop r15
.pool
.endfunc
.else
.pool
.endif
.endarea

.ifdef NameMaxLength
.org 0x0802D2F0
.area 0x0802D338-.,0x00
.func NamingScreenUpdateName
push r4-r5,r14

; These are the utility params.
ldr r4,=0x030018BC
; Source string.  We compose to save bytes.
mov r0,r4
add r0,0x0300199C - 0x030018BC
; Temp buffer.
ldr r5,=0x030041DC

str r5,[r4,8]
str r0,[r4,12]

mov r0,7
strb r0,[r4,5]
mov r0,6
; Center based on 5 tiles.
mov r2,NameMaxTiles * 8
bl CopyString8x8CenterR0

; Now copy that to the screen.
mov r0,0xC0
strh r0,[r4,4]
str r5,[r4,8]

ldr r0,=0x04000008
ldrh r1,[r0]
; Check the correct BG charblock.
mov r0,0x000C
and r0,r1
lsl r0,r0,12

ldr r1,=0x06002800
add r0,r0,r1
str r0,[r4,12]
bl 0x0806B514

pop r4-r5,r15
.pool
.endfunc
.endarea
.endif

; We also need to change the highlight func to do all 5 tiles now.
.org 0x0802E5DC
.area 0x0802E694-.,0x00
.func NamingScreenQueueHighlight
push r4-r6,r14

; I think this tells the queue there are things to process.  Get it out of the way.
ldr r0,=0x030018CC
mov r6,1
strb r6,[r0,3]

; And the font drawing struct.
ldr r4,=0x030018BC
; 1 means set 0xF000 on the tile, 2 means 0xD000 (red palette.)
ldrb r0,[r4,1]
mov r1,0xD0
cmp r0,1
bne @@keepRed
mov r1,0xF0
@@keepRed:
strb r1,[r4,1]

; Grab the screen base block (bg tilemap) and translate to a byte offset.
ldr r0,=0x04000008
ldrh r0,[r0]
mov r5,0x1F;
lsl r5,r5,8
and r5,r0
lsl r5,r5,3

; This is the naming screen param struct.
ldr r3,=0x03001AE5
; This is the y position.  Translate to bytes, and add to r5 (will soon be our dest tilemap address.)
ldrb r0,[r3,6]
lsl r0,r0,7
add r5,r5,r0

; Base address for our x and y positions.  Now dest is just missing xpos.
ldr r0,=0x0600010E
add r5,r5,r0

; Time for x, let's also figure how many tiles need to be changed - r6 is already 1.
ldrb r0,[r3,7]
cmp r0,4
bls @@noGapOffset

; Account for the gap after the first 5 letters.
add r0,r0,1
cmp r0,10
bls @@noGapOffset

; Position 10 (adjusted to 11 above) means the buttons on the right.  And another gap.
add r0,r0,1
; 5 tiles total to update.
mov r6,5

@@noGapOffset:
; Translate X to bytes.
lsl r0,r0,1
add r5,r5,r0
; Not sure if this is important... we overwrite part of it with a 1 soon.  From original code.
strh r0,[r4,4]

@@nextTile:
mov r0,1
strb r0,[r4,4]
str r5,[r4,8]
add r5,r5,2
bl 0x0802FD58
sub r6,r6,1 ; Sets Z/eq on zero.
bne @@nextTile

pop r4-r6,r15
.pool
.endfunc
.endarea

; This updates the message below the keyboard.
; We alter to support our new keyboard numbers and layout.
.org 0x0802E578
.area 0x0802E5DC-.,0x00
.func NameScreenUpdateMessage
push r4-r6,r14
; Font drawing params.
ldr r4,=0x030018BC
; These are the naming screen params, 11 is the current message.
ldr r2,=0x03001AE5
ldrb r0,[r2,11]

; This is the new message to set.
ldrb r1,[r4,4]
cmp r0,r1
beq @@skip

; Update the current message.
strb r1,[r2,11]

ldr r0,=@MessageText
; Probably too much space, but easier than multiplying.
lsl r1,r1,5
add r0,r1

; Let's figure out how long that is, to center it.
mov r1,32
mov r2,1
bl Calc8x8PixelWidth
; We still have str in r0, r1 is now actual length, and r2 pixel width.
strb r1,[r4,5]
str r0,[r4,12]

; Our temp buffer for drawing.
ldr r5,=0x030041DC
str r5,[r4,8]

; Actual clear size, we'll let this center for us based on r2.
mov r0,18
bl CopyString8x8CenterR0

; It's nicely centered in those 18 blocks, time to copy to screen.
mov r0,0x90
lsl r0,r0,2
strh r0,[r4,4]
str r5,[r4,8]

ldr r0,=0x04000008
ldrh r1,[r0]
; Check the correct BG charblock.
mov r0,0x000C
and r0,r1
lsl r0,r0,12
.ifdef NameMaxLength
	ldr r1,=0x060028C0
.else
	ldr r1,=0x06002880
.endif
add r0,r1
str r0,[r4,12]
bl 0x0806B4B4

@@skip:
pop r4-r6,r15
.pool
.endfunc
.endarea

; This function draws the cursor after the name.
.org 0x0802E2CC
.area 0x0802E344-.,0x00
.func NamingScreenUpdateCursor
push r4-r5,r14
; Utility parameters.
ldr r4,=0x030018BC

; Check the current frame number (I suspect) to make the cursor blink.
ldr r0,=0x030018D6
ldrh r0,[r0]
mov r1,0x10
tst r0,r1
bne @@disableCursor

; Naming screen parameters.
ldr r5,=0x03001AE5
; Let's check if we're in the Yes/No confirmation select - hide the cursor if so.
ldrb r0,[r5,2]
cmp r0,0xFF
bne @@disableCursor

; Okay, grab the current name length.  Decrease if we're at the end.
.ifdef NameMaxLength
	bl NamingScreenCanAddMore
	ldrb r1,[r5,4]
	cmp r0,0
	bne @@skipDecrement
.else
	ldrb r1,[r5,4]
	cmp r1,4
	blo @@skipDecrement
.endif
sub r1,r1,1
@@skipDecrement:

; This buffer has the string so far.
ldr r0,=0x0300199C
mov r2,0
bl Calc8x8PixelWidth

; r2 now has the actual pixel width.  Add the start offset.
.ifdef NameMaxLength
	add r2,0x65
.else
	add r2,0x69
.endif
strb r2,[r4,4]
mov r0,9
strb r0,[r4,5]
mov r0,0xF0
strb r0,[r4,9]
mov r0,0x90
strb r0,[r4,8]
mov r0,0
b @@drawCursor

@@disableCursor:
mov r0,0
strh r0,[r4,4]
strh r0,[r4,8]

@@drawCursor:
strb r0,[r4,1]
bl 0x0806A3B0

pop r4-r5,r15
.pool
.endfunc
.endarea

.ifdef NameMaxLength
; 0802E074 is called when A is pressed.  We replace the max length checks in part of it.
; This is the check for if a letter can be added.
.org 0x0802E15C
.area 0x0802E164-.,0x00
; Should return r0=1/0.
bl NamingScreenCanAddCurrent
cmp r0,0
bne 0x0802E16C
.endarea
.org 0x0802E16C
ldrb r0,[r7,5]

; This part checks if the name is full to move straight to OK.
.org 0x0802E190
.area 0x0802E1A2-.,0x00
; Should return r0=1/0.
bl NamingScreenCanAddMore
cmp r0,0
; Not full, don't move.
bne 0x802E1A2

; Update X and Y.
mov r0,10
strb r0,[r7,7]
mov r0,5
strb r0,[r7,6]
nop
.endarea

.org 0x0802E00C
.area 0x0802E074-.,0x00
; Args: uint8_t letterIndex
.func NamingScreenAddLetter
push r14
; Translate to an actual letter straight away.
bl NamingScreenGetLetter

; Naming screen params, for the current length.
ldr r3,=0x03001AE5
; This is the name buffer.
ldr r2,=0x0300199C
ldrb r1,[r3,4]
strb r0,[r2,r1]
; And increase the length.
add r1,r1,1
strb r1,[r3,4]

bl NamingScreenUpdateName

pop r15
.endfunc

; Args: uint8_t letterIndex
; Returns: char letter
.func NamingScreenGetLetter
; Naming screen params.  Get the keyboard #.
ldr r3,=0x03001AE5
ldrb r1,[r3,3]

cmp r1,1
beq @@lowercase
blo @@uppercase

ldr r1,=0x0862C14C
b @@keysReady

@@uppercase:
ldr r1,=0x0862C0D4
b @@keysReady

@@lowercase:
ldr r1,=0x0862C110

@@keysReady:
; Grab the letter at the letter index.
ldrb r0,[r1,r0]
bx r14
.pool
.endfunc
.endarea
.endif
