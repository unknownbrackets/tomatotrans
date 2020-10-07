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
bl 0x080749B8 ; Draw8x8FixedStrIndexedToVRAM

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
bl 0x080749B8 ; Draw8x8FixedStrIndexedToVRAM

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
bl 0x080749B8 ; Draw8x8FixedStrIndexedToVRAM

add sp,4
pop r4-r5
pop r0
bx r0
.pool
.endfunc
.endregion
