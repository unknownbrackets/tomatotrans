; This function initially draws the list items to a buffer.
; We extend it to use 10 tiles per item, not 8.
.org 0x080778FC
.region 0x080779A8-.,0x00
.func MenuItemsDrawList
push r4-r7,r14
add sp,-4

ldr r7,=0x03004FF8
; Prep the current scroll offset.
ldrb r6,[r7,15]

; Clear the buffer to a background color.
ldr r0,=0x99999999
str r0,[sp,0]
add r0,sp,0
ldr r1,[r7,8]
ldr r2,=0x05000000 | (13 * 10 * 32 / 4)
bl 0x080942E4 ; CpuSet

mov r4,0
b @@next

@@continue:
ldr r1,=0x08630E5C + 8
sub r0,r5,1
lsl r0,r0,4
ldrb r1,[r0,r1]

mov r0,0x1F
and r0,r1
cmp r0,0x1F
beq @@break

; Check if the item is usable.
mov r0,r5
bl 0x08075040

mov r2,2
cmp r0,0
beq @@skipEnable
mov r2,0
@@skipEnable:

; Multiply by 320 for 10 tiles per.
lsl r0,r4,8
lsl r1,r4,6
add r0,r0,r1
ldr r1,[r7,8]
add r1,r1,r0

; Now draw the item name.
ldr r0,=0x084903F0
ldr r3,[r0,0]
ldr r0,[r0,4]
str r0,[sp,0]
mov r0,r5
bl Draw8x8FixedStrIndexedToVRAM

add r4,r4,1
add r6,r6,1

cmp r4,12
bhi @@break

@@next:
ldr r0,[r7,8]
lsl r1,r6,1
ldr r2,=0x5698
add r0,r0,r2
ldrb r5,[r0,r1]
cmp r5,0
bne @@continue

@@break:
add sp,4
pop r4-r7
pop r0
bx r0
.pool
.endfunc
.endregion

; Patch the func that handles updates to use 10 instead of 8.
.org 0x080779C0
mov r0,10

; Move the quantity over to the right to make space.
.org 0x0807766A
mov r0,0xD8

; This function updates the buffer on scroll up.
.org 0x080779E4
.region 0x08077A54-.,0x00
.func MenuItemsScrollListUp
push r4-r6,r14
add sp,-4

mov r4,12
ldr r6,=0x03004FF8

; Loop over each item to move it up.
; We can't copy in one go, because CpuFastSet is not overlap safe.
@@upLoop:
lsl r1,r4,8
lsl r0,r4,6
add r1,r1,r0
ldr r0,[r6,8]
add r0,r0,r1

mov r2,0x80
add r2,0xC0

add r1,r0,r2
mov r2,0x50
bl 0x080942E0 ; CpuFastSet

sub r4,r4,1
cmp r4,0
bge @@upLoop

ldr r1,[r6,8]
ldrb r0,[r6,15]
lsl r0,r0,1

ldr r2,=0x5698
add r1,r1,r2
ldrb r0,[r1,r0]

; Check if the item is usable.
mov r5,r0
bl 0x08075040

mov r2,2
cmp r0,0
beq @@skipEnable
mov r2,0
@@skipEnable:

; And draw the item name to the buffer.
ldr r1,[r6,8]
ldr r0,=0x084903F0
ldr r3,[r0,0]
ldr r0,[r0,4]
str r0,[sp,0]
mov r0,r5
bl Draw8x8FixedStrIndexedToVRAM

add sp,4
pop r4-r6
pop r0
bx r0
.pool
.endfunc
.endregion

; This function updates the buffer on scroll down.
.org 0x08077A54
.region 0x08077AB4-.,0x00
.func MenuItemsScrollListDown
push r4-r5,r14
add sp,-4

ldr r4,=0x03004FF8

; Copy all the lines up one line in the buffer.
; We can do this all at once because it copies sequentially.
ldr r1,[r4,8]
mov r0,r1
add r0,0x80
add r0,0xC0

mov r2,0x3C
lsl r2,r2,4
bl 0x080942E0 ; CpuFastSet

ldr r1,[r4,8]

ldrb r0,[r4,15]
add r0,12
lsl r0,r0,1

ldr r2,=0x5698
add r1,r1,r2
ldrb r0,[r1,r0]

; Check if the item is usable.
mov r5,r0
bl 0x08075040

mov r2,2
cmp r0,0
beq @@skipEnable
mov r2,0
@@skipEnable:

; Skip to the end of the buffer.
ldr r1,[r4,8]
mov r0,0xF0
lsl r0,r0,4
add r1,r1,r0

; Draw the item title.
ldr r0,=0x084903F0
ldr r3,[r0,0]
ldr r0,[r0,4]
str r0,[sp,0]
mov r0,r5
bl Draw8x8FixedStrIndexedToVRAM

add sp,4
pop r4-r5
pop r0
bx r0
.pool
.endfunc
.endregion


; Similar to the above, this is shop items (and clothing.)
; We adjust as well to allow longer item names.
.org 0x0807DED0
.region 0x0807DF7C-.,0x00
.func ShopItemsDrawList
push r4-r7,r14
add sp,-8

; Clear the buffer to the background color.
ldr r0,=0x99999999
str r0,[sp,0]
ldr r4,=0x03004FF8
ldr r1,[r4,8]
; Should be safe even for clothing.
ldr r2,=0x05000000 | (13 * 10 * 32 / 4)
add r0,sp,0
bl 0x080942E4 ; CpuSet

mov r6,0

ldr r0,=0x03005038
ldr r7,[r0]

@@nextItem:
mov r0,0x2D
ldrb r1,[r4,r0]
add r1,r1,r6

lsl r0,r1,1
add r0,r0,r1
lsl r0,r0,2
add r0,r7,r0

ldrb r2,[r0,4]
cmp r2,0
beq @@return

ldrb r0,[r0,14]
mov r5,2
cmp r0,0
beq @@skipEnable
mov r5,0
@@skipEnable:

ldr r1,[r4,8]

; Multiply by 320 not 256.
lsl r0,r6,8
lsl r3,r6,6
add r0,r0,r3
add r1,r1,r0

ldrb r0,[r7,1]
cmp r0,1
bne @@clothing

ldr r0,=0x084903F0
b @@skipClothing

@@clothing:
ldr r0,=0x08490408

@@skipClothing:
ldr r3,[r0,0]
ldr r0,[r0,4]
str r0,[sp,0]
mov r0,r2
mov r2,r5
bl Draw8x8FixedStrIndexedToVRAM
add r6,r6,1
cmp r6,5
bls @@nextItem

@@return:

add sp,8
pop r4-r7,r15
.pool
.endfunc
.endregion

.org 0x0807DF7C
.region 0x0807DFB4-.,0x00
.func ShopItemsListParams
push r15
add sp,-8

; Move r0 to r2 already for 080306E8.
mov r2,r0

mov r1,sp

mov r0,2
strh r0,[r1,0]

mov r0,8
strh r0,[r1,2]

mov r0,10
strh r0,[r1,4]

mov r0,6
strh r0,[r1,6]

ldr r0,=0x03004FF8
ldr r1,[r0,8]
mov r0,0
mov r3,sp
bl 0x080306E8

add sp,8
pop r15
.pool
.endfunc
.endregion

; This is from shop items, when scrolling up.
.org 0x0807DFB4
.region 0x0807E04C-.,0x00
.func ShopItemsScrollListUp
push r4-r7,r14
add sp,-4

mov r4,5
ldr r6,=0x03004FF8
ldr r0,=0x03005038
ldr r5,[r0]

; This indicates which type of items we're looking at.
ldrb r7,[r5,1]

@@upLoop:
ldr r0,[r6,8]

mov r2,0x80
add r2,0xC0

lsl r1,r4,8
lsl r3,r4,6
add r0,r0,r1
add r0,r0,r3
add r1,r0,r2

mov r2,0x50
bl 0x080942E0 ; CpuFastSet

sub r4,1
cmp r4,0
bge @@upLoop

mov r0,0x2D
ldrb r1,[r6,r0]

lsl r0,r1,1
add r0,r0,r1
lsl r0,r0,2
add r4,r5,r0

ldrb r5,[r4,4]
cmp r7,1
bne @@clothingType

ldr r0,=0x084903F0
b @@doneType

@@clothingType:
ldr r0,=0x08490408

@@doneType:
ldr r3,[r0,0]
ldr r1,[r0,4]
str r1,[sp,0]

ldrb r0,[r4,14]
mov r2,2
cmp r0,0
beq @@skipEnable
mov r2,0
@@skipEnable:

ldr r1,[r6,8]
mov r0,r5
bl Draw8x8FixedStrIndexedToVRAM

add sp,4
pop r4-r7,r15
.pool
.endfunc
.endregion

; This is from shop items, when scrolling down.
.org 0x0807E04C
.region 0x0807E0D8-.,0x00
.func ShopItemsScrollListDown
push r4-r7,r14
add sp,-4

ldr r5,=0x03004FF8
ldr r1,[r5,8]

ldr r0,=0x03005038
ldr r6,[r0]
ldrb r7,[r6,1]

mov r2,0x80
add r2,0xC0
add r0,r1,r2
add r2,0x50
bl 0x080942E0 ; CpuFastSet

mov r4,0x2D
ldrb r4,[r5,r4]
add r1,r4,5

lsl r0,r1,1
add r0,r0,r1
lsl r0,r0,2
add r4,r0,r6

cmp r7,1
bne @@clothingType

; Item names.
ldr r0,=0x084903F0
b @@typeDone

@@clothingType:
ldr r0,=0x08490408

@@typeDone:
ldr r3,[r0,0]
ldr r0,[r0,4]
str r0,[sp,0]

ldrb r0,[r4,14]
mov r2,2
cmp r0,0
beq @@skipEnable
mov r2,0
@@skipEnable:

ldr r1,[r5,8]
mov r0,0x64
lsl r0,r0,4
add r1,r1,r0
ldrb r0,[r4,4]
bl Draw8x8FixedStrIndexedToVRAM

add sp,4
pop r4-r7,r15
.pool
.endfunc
.endregion
