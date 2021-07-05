; This function sets up the equipment selection list.
; We extend it to use 10 tiles per item, instead of 8.
.org 0x0807A3D4
.region 0x0807A410-.,0x00
.func MenuEquipmentListParams
push r14
add sp,-8

; Move r0 to r2 already for 080306E8.
mov r2,r0

mov r3,sp

mov r0,15
strh r0,[r3,0]

mov r0,1
strh r0,[r3,2]

mov r0,10
strh r0,[r3,4]

mov r0,6
strh r0,[r3,6]

ldr r0,=0x03004FF8
ldr r1,[r0,8]
mov r0,0
bl 0x080306E8

add sp,8
pop r15
.pool
.endfunc
.endregion

; This function loads clothing for a given character and draws names.
; We extend it to use 10 tiles per item, not 8.
.org 0x0807A310
.region 0x0807A3D4-.,0x00
.func MenuEquipmentDrawList
mov r3,r8
push r3-r7,r14
add sp,-4

; Prep constant 0x2E78.
mov r0,0x2E
lsl r0,r0,8
add r0,0x78

; Place a 0 into the stack.
mov r3,0
str r3,[sp,0]

; And now a size parameter for CpuSet into r2.
mov r2,0x01
lsl r2,r2,24
add r2,0x10

; Menu params struct, and list buffer pointer at 8.
ldr r5,=0x03004FF8
ldr r1,[r5,8]
add r1,r0
; Save the equipment list buffer ptr in r8.
mov r8,r1

; Zero out all 32 items.
add r0,sp,0
bl 0x080942E4 ; CpuSet

; Grab the selected character.
ldrb r5,[r5,25]
; Prep an index into the available equipment list at r8.
mov r4,0
; And now a possible clothing item index (global list of what the player has.)
mov r6,0

; This is where the list of equipment the player has is kept.
ldr r7,=0x03001A70

@@nextClothing:
; The clothing list has two bytes, we want the first (clothing ID.)
add r1,r7,r6
ldrb r1,[r1,r6]
mov r0,r5

; Is this equippable (including, not equipped on someone else)?
bl 0x080756FC
cmp r0,0
beq @@skipAdd

; Yes, equippable.  Add to the list starting at r8 at index r4.
mov r0,r8
; Reload the clothing ID from the clothing list.
add r1,r7,r6
ldrb r1,[r1,r6]
strb r1,[r0,r4]
add r4,r4,1

@@skipAdd:
add r6,1
cmp r6,0x20
blo @@nextClothing

; Not sure what this does, seems to update the list.  Takes the character number.
mov r0,r5
bl 0x0808127C

; Grab the menu params struct address again.
ldr r3,=0x03004FF8

; Store the count of items in there.
strb r4,[r3,30]
; And grab the offset our scroll is at.
ldrb r4,[r3,29]

; Now grab the buffer pointer, which we'll use in the upcoming loop.
ldr r5,[r3,8]

; Grab the clothing names string block.
ldr r7,=0x08490408
; And put the size info on the stack.
ldr r0,[r7,4]
str r0,[sp,0]
; Then all we need is the string pointer start.
ldr r7,[r7,0]

; Okay, now go through the display by y and draw each item.
mov r6,0

@@nextLine:
; Grab the clothing ID for the name lookup.
mov r0,r8
ldrb r0,[r0,r4]
add r4,r4,1

; Multiply y by 320 for 10 tiles per line.
lsl r2,r6,8
lsl r1,r6,6
add r1,r1,r2
; Then tack that onto the buffer.
add r1,r1,r5

; Standard colors, grab the string start pointer.
mov r2,0
mov r3,r7

bl Draw8x8FixedStrIndexedToVRAM

; Alright, on to the next one.
add r6,r6,1
cmp r6,6
blo @@nextLine

add sp,4
pop r3-r7
mov r8,r3
pop r0
bx r0
.pool
.endfunc
.endregion

; This function updates the buffer on scroll up.
.org 0x0807A410
.region 0x0807A470-.,0x00
.func MenuEquipmentScrollListUp
push r4-r6,r14
add sp,-4

; Grab the menu params struct and the buffer pointer from it.
ldr r4,=0x03004FF8
ldr r5,[r4,8]
; And start a counter off at 5.
mov r6,5

; Loop over each item to move it up.
; We can't copy in one go, because CpuFastSet is not overlap safe.
@@upLoop:
; Grab the current item's offset to move this item down from.
lsl r0,r6,8
lsl r1,r6,6
add r0,r0,r1
; And add in the buffer pointer.
add r0,r0,r5

; Now the destination, which is 0x140 (10 x 32) later.
mov r1,r0
add r1,0x80
add r1,0xC0

; And now we copy 10 * 8 * 4 bytes.
mov r2,0x50
bl 0x080942E0 ; CpuFastSet

sub r6,r6,1 ; Sets N and V flags.
bge @@upLoop

; Grab the scroll offset from the menu params struct.
ldrb r0,[r4,29]
; And the offset into the buffer for the list clothing IDs.
ldr r3,=0x2E78
add r0,r0,r3
ldrb r0,[r5,r0]

; Put the buffer in place as a parameter.
mov r1,r5

; Grab the clothing name text block, store size on the stack.
ldr r3,=0x08490408
ldr r2,[r3,4]
str r2,[sp,0]
ldr r3,[r3,0]

mov r2,0
bl Draw8x8FixedStrIndexedToVRAM

add sp,4
pop r4-r6
pop r0
bx r0
.pool
.endfunc
.endregion

; This function updates the buffer on scroll down.
.org 0x0807A470
.region 0x0807A4B8-.,0x00
.func MenuEquipmentScrollListDown
push r4,r14
add sp,-4

ldr r4,=0x03004FF8

; Copy all the lines up one line in the buffer.
; We can do this all at once because it copies sequentially.
ldr r1,[r4,8]
mov r0,r1
add r0,0x80
add r0,0xC0

; 0x190 = 5 items * 10 tiles per  item * 32 bytes per tile / 4 bytes per word.
mov r2,0x19
lsl r2,r2,4
bl 0x080942E0 ; CpuFastSet

; Reload the buffer pointer.
ldr r1,[r4,8]

; Grab the scroll offset.
ldrb r0,[r4,29]
; And find and read the item IDs portion of the buffer.  Add 5 for the new one at the bottom.
ldr r2,=0x2E78 + 5
add r0,r0,r2
ldr r0,[r1,r0]

; Now offset buffer to where we're drawing to, 5 * 10 * 32 bytes later.
mov r2,0x19
lsl r2,r2,6
add r1,r1,r2

; Grab the clothing name text block, store size on the stack.
ldr r3,=0x08490408
ldr r2,[r3,4]
str r2,[sp,0]
ldr r3,[r3,0]

mov r2,0
bl Draw8x8FixedStrIndexedToVRAM

add sp,4
pop r4
pop r0
bx r0
.pool
.endfunc
.endregion

; 0807A0EC updates sprites on clothing selection.  Here we adjust to give space for "Defense".
; We just move each sprite right 16 pixels.
.org 0x0807A27E
mov r0,0xB8
.org 0x0807A28A
mov r0,0xB8
.org 0x0807A29C
mov r0,0xD8
.org 0x0807A2A8
mov r0,0xD8
.org 0x0807A2B8
mov r1,0xC0
.org 0x0807A2C4
mov r1,0xC0
