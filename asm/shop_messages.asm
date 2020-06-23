; This updates the item descriptions.  We patch to give more space for the text.
.org 0x0807D2E8
.area 0x0807D4BC-.,0x00
.func ShopUpdateItemDescription
mov r3,r8
push r3-r7,r14
add sp,-8
; Start by moving dest down 16 tiles and right 2.  Happens to be a 0x4040 offset.
mov r1,0x40
lsl r2,r1,8
orr r1,r2
; And let's put that in r7.  This is a our main draw dest now.
add r7,r0,r1

; This seems to hold a pointer to shop parameters and items.
ldr r0,=0x03005038
ldr r2,[r0,0]
; This offset holds what I think is the current selection.
ldr r0,=0x03004FF8 + 0x2C
ldrb r1,[r0]
; (x * 3) * 4 = x * 12
lsl r0,r1,1
add r0,r0,r1
lsl r0,r0,2

; The item's ID is the 4th byte in the 12 byte structure, put it in r6.
; Note that this is 1 indexed, not 0 indexed...
add r0,r0,r2
ldrb r6,[r0,4]

; In shopParams, the byte at 1 is 1 for items, clothing otherwise.
; The byte at 2 is 0 when we shouldn't show any description.
ldrb r0,[r2,1]
ldrb r4,[r2,2]

; This is the background to clear using a few times.
ldr r1,=0x99999999
str r1,[sp,4]

cmp r0,1
bne @@clothingShop

; This is a regular consumable item shop.  Grab the item description text block.
ldr r3,=0x08490448
; Prepare the destination in r1, we'll need it later.
mov r1,r7

cmp r4,0
beq @@itemClearDesc

; Okay, draw that item's description.
ldr r0,[r3,4]
str r0,[sp,0]
; Our index is 1 offset, so subtract one.
sub r0,r6,1
mov r2,0
ldr r3,[r3,0]
; This func overrides the description for a couple items.
bl 0x08074DD0
b @@return

@@itemClearDesc:
; Setup the word size for the clear.
; r0 is already 1.  Move the bit to 24 to tell CpuFastSet to use a fixed source.
lsl r0,r0,24
; Byte 6 is the clear size, which might be zero.
ldrb r4,[r3,6]
cmp r4,0
bne @@itemDescHasClear
ldrb r4,[r3,4]
@@itemDescHasClear:
; Convert tiles to words.
lsl r4,r4,3
orr r4,r0

; r1 is already dest.  Call CpuFastSet, since we're doing tiles (32 bytes / 8 words.)
add r0,sp,4
mov r2,r4
bl 0x080942E0

add r0,sp,4
; Add one tile vertically to dest for the next clear.
mov r1,1
lsl r1,r1,10
add r1,r1,r7
mov r2,r4
bl 0x080942E0
b @@return

@@clothingShop:
; This is the clothing data - offset by one item since our number is 1 indexed.
ldr r2,=0x0863146C - 0x10
lsl r1,r6,4
add r1,r1,r2
; Now grab the flags.  First is who can equip, second is status effects which we save for later.
ldrb r0,[r1,12]
ldrb r1,[r1,13]
mov r8,r1

cmp r0,0x0F
beq @@equipAnyChar

; Okay, at this point we need to go through all characters and show which ever can wear it.
; Only one character can wear it if it wasn't 0x0F.
mov r5,0

@@equipNextChar:
; Is this equippable (including, not equipped on someone else)?
mov r0,r5
mov r1,r6
bl 0x080756FC
cmp r0,0
bne @@foundEquipChar
add r5,r5,1
cmp r5,4
blo @@equipNextChar

; If none matched, fall through to clear.
@@equipAnyChar:
; This was originally a DMA3 transfer, and everything else CpuSet.
; We just use CpuFastSet to be consistent.
add r0,sp,4
mov r1,r7
; Instead of going to the block info, let's just clear a fixed 13 tiles.
; We position the status text in a fixed place anyway.
mov r2,0x68
; Add the fixed source bit and we're done.
mov r3,1
lsl r3,r3,24
orr r2,r3
bl 0x080942E0
b @@equipDone

@@foundEquipChar:
; These are the utility/font drawing params.
ldr r6,=0x030018BC
str r7,[r6,8]

.ifdef NameMaxLength
	mov r0,r5
	; Since the length can vary, the next part might vary too... just clear more.
	mov r1,13
	bl CopyCharName8x8ToVRAMClearR1
.else
	; Need to get the name, this is the char data base.
	ldr r3,=0x03001EDC
	lsl r0,r5,1
	add r0,r0,r5
	lsl r0,r0,5
	add r0,r0,r3
	str r0,[r6,12]

	mov r0,4
	strb r0,[r6,5]
	mov r0,13
	bl CopyString8x8ClearR0
.endif

; This is the pixel width of the name, first get the offset into the tile.
ldrb r1,[r6,1]
; Add a space.
add r1,r1,5
mov r2,7
and r2,r1
bic r1,r2
; Okay, now convert to tiles and add to dest.
lsl r1,r1,2
add r1,r1,r7
str r1,[r6,8]

; Grab the text block for can equip.
ldr r0,=0x084905E8
ldr r3,[r0,0]
str r3,[r6,12]
ldrb r0,[r0,4]
strb r0,[r6,5]

; Our VWF reads the font offset from here.  Don't clear, would erase last part of name.
ldr r0,=MFontClearSize
mov r1,1
neg r1,r1
strh r1,[r0,0]
strb r2,[r0,MFontXOffset-MFontClearSize]

; Now we draw that after the name.
bl CopyString8x8ToVRAM

@@equipDone:
; Move dest right 13 tiles - add 13 * 16 twice to get 13 * 32.
add r7,0xD0
add r7,0xD0

; Now we're going to draw the status effects this equipment has.]
; There might be 2, so r5=effectNum and r6=yPosition.
mov r5,0
mov r6,0

; This is the description flag we loaded at the beginining.
cmp r4,0
beq @@effectsDone

; We kept the effect flags here, we'll check the bits one by one.
mov r4,r8

@@nextEffect:
lsr r4,r4,1 ; Sets C/cs if the bit was 1, cc if it was 0
bcc @@skipEffect

; This is the clothing status effect block.
ldr r0,=0x084903D8
ldr r3,[r0,0]
ldr r0,[r0,4]
str r0,[sp]

; Offset dest by y position.
lsl r1,r6,10
add r1,r1,r7

; And now draw the text.
mov r0,r5
mov r2,0
bl 0x080748BC
add r6,r6,1

@@skipEffect:
add r5,r5,1
cmp r5,5
blo @@nextEffect

@@effectsDone:
; Now we need to clear any remaining lines from old status effect text.
ldr r0,=0x084903D8
ldrb r4,[r0,6]
cmp r4,0
bne @@effectNameHasClear
ldrb r4,[r0,4]
@@effectNameHasClear:
; Convert to words.
lsl r4,r4,3

; We'll use this 1 later too, but using it now to set the fixed source bit.
mov r5,1
lsl r0,r5,24
orr r4,r0

cmp r6,1
bhi @@return
beq @@skipClearStatus1

; Here we clear line 1 using CpuFastSet.  Guess there were no effects.
add r0,sp,4
mov r1,r7
mov r2,r4
bl 0x080942E0

@@skipClearStatus1:

; Clear line 2.
add r0,sp,4
; r5 has a 1 in it, so offset Y by that much.
lsl r1,r5,10
add r1,r1,r7
mov r2,r4
bl 0x080942E0

@@return:
add sp,8
pop r3-r7
mov r8,r3
pop r0
bx r0
.pool
.endfunc
.endarea
