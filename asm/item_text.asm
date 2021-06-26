; We want longer item names and descriptions.
; Normally names are embedded in a 16 byte struct, and descriptions at 0x1A.
; Names originally at 0x08630E5C and descriptions at 0x08639558.

; See also battle_spoils.asm.
@NameLen equ 0x10
@DescLen equ 0x30

; For names, the game already multiplies by 16.  We just adjust the base and length.

; For descriptions, it normally uses a buffer (see battle_message.asm.)
; We adjust to store a pointer instead, and change the multiplier.

; 0804AB90 draws the item name directly without a buffer.
.org 0x0804AC94
dw org(ItemTextName)
.org 0x0804AC32
db @NameLen
; We need to force it only to clear 8, though.
.org 0x0804AC3C
bl CopyString8x8Clear10
.org 0x0804AC9C
dw 0x06000CC0
.org 0x0804AC5E
mov r0,4

; 0806B27C is used to draw text in dialogs, but is handled in dialog_item.asm.
; That's called when [ITEM] is used in dialog text.

; 0804EBF0 has all four description reads, each into a buffer as noted above.
.org 0x0804ECD4
dw org(ItemTextDesc)
.org 0x0804ECA8
db @DescLen
.org 0x0804ECB4
; Replace the memcpy with a store of the pointer.
.area 0x0804ECBA-.,0x00
; r0 was the 0x03000BFC pointer + 2, add another 2 for the pointer loc.
add r0,r0,2
str r1,[r0,0]
nop
.endarea

.org 0x0804EE10
dw org(ItemTextDesc)
.org 0x0804EDD4
db @DescLen
.org 0x0804EDE0
; Replace the memcpy with a store of the pointer.
.area 0x0804EDE6-.,0x00
; r0 was the 0x03000BFC pointer + 2, add another 2 for the pointer loc.
add r0,r0,2
str r1,[r0,0]
nop
.endarea

.org 0x0804EE98
dw org(ItemTextDesc)
.org 0x0804EE36
db @DescLen
.org 0x0804EE42
; Replace the memcpy with a store of the pointer.
.area 0x0804EE48-.,0x00
; r0 was the 0x03000BFC pointer + 2, add another 2 for the pointer loc.
add r0,r0,2
str r1,[r0,0]
nop
.endarea

.org 0x0804EF50
dw org(ItemTextDesc)
.org 0x0804EED4
db @DescLen
.org 0x0804EEE0
; Replace the memcpy with a store of the pointer.
.area 0x0804EEE6-.,0x00
; r0 was the 0x03000BFC pointer + 2, add another 2 for the pointer loc.
add r0,r0,2
str r1,[r0,0]
nop
.endarea

; 08074DD0 used to patch in [NAME3], we now support directly.
; We just remove the patching part of it.
.org 0x08074DEE
.region 0x08074E24-.,0x00
b 0x08074E24
.endregion

; Key items are handled using the same strings, but drawn in 0807C2F4.
; We patch just the part of it that draws strings to a temp buffer, to give more space per item.
.org 0x0807C398
.region 0x0807C3BA-.,0x00
; Entry: r0=tempBufferPtr, r1=FREE, r2=FREE, r3=FREE, r4=FREE, r5=itemID, r6=pos, r7=inventoryIndex, r8=inventoryList
; Must have on exit: r0=FREE, r1=FREE, r2=FREE, r3=FREE, r4=FREE, r5=FREE, r6=pos, r7=inventoryIndex, r8=inventoryList

; We multiply by 0x140 instead of 0x100 here.
lsl r2,r6,8
lsl r3,r6,6
add r2,r2,r3

; And now get the temp buf address and add.
ldr r1,[r0]
add r1,r1,r2

; Load the item names text block.
ldr r0,[0x0807C3EC] ; 0x084903F0
ldr r3,[r0,0]
ldr r4,[r0,4]
str r4,[sp,0]

; Now draw the item name from r5.
mov r0,r5
mov r2,0
bl Draw8x8FixedStrIndexed

add r7,r7,1
add r6,r6,1
.endregion

; This function copies those key items to the actual screen.
.org 0x0807C268
.region 0x0807C2F4-.,0x00
.func MenuKeyItemsDrawSpecialList
push r4-r7,r14

; Grab the menu params struct for the buffer info and the target VRAM.
ldr r0,=0x03004FF8

; Load the target VRAM into r4.  Offset by 0x1080 (0x21 << 7) as a starting position.
ldr r4,[r0,0]
mov r1,0x21
lsl r1,r1,7
add r4,r4,r1

; The pointer at [r0,8] has the POINTER to a temp buffer at an offset of 0x5734.
ldr r0,[r0,8]
ldr r1,=0x5734
; Okay, now load the temp buffer pointer into r5.
ldr r5,[r0,r1]

; Now prepare the DMA3 address.
ldr r6,=0x040000D4
; And also the trigger value (to copy 0x140 bytes = 0x50 words.)
ldr r7,=0x84000050

; We'll keep x in r2, and y in r3.
mov r2,0

@@nextX:
mov r3,0

@@nextY:
; Prep the target with our Y offset.
; We don't add this directly to r4, since it resets for column 2.
lsl r0,r3,10
add r0,r0,r4

; That's it, now just write the DMA3 registers.
str r5,[r6,0]
str r0,[r6,4]
str r7,[r6,8]

; Move src forward to the next item (add 0x140 = 10 tiles.)
add r5,0x80
add r5,0xC0

; More to go in this column?
add r3,r3,1
cmp r3,11
blo @@nextY

; Alright, shift our target VRAM to the right column (0x1A0 = 13 tiles.)
add r4,0xD0
add r4,0xD0

; But only loop if we did the first column just now.
add r2,r2,1
cmp r2,2
blo @@nextX

pop r4-r7,r15
.pool
.endfunc
.endregion
