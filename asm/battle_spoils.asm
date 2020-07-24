; Could move these.
@ItemTextPtr equ 0x08639CAD
@ItemTextLen equ 4
@FoundTextPtr equ 0x08639CB1
@FoundTextLen equ 6

; See also item_text.asm.
@NameLen equ 0x10

; This is part of the larger function, 08050E2C, and shows the spoils.
; All regs are free at this point.
.org 0x08050F6E
.region 0x0805100C-.,0x00
; Drawing params structure.
ldr r7,=0x030018BC
; Start position for drawing.
ldr r2,=0x06000C20
mov r9,r2

; Draw 'Item' first off.
ldr r0,=@ItemTextPtr
str r0,[r7,12]
mov r0,@ItemTextLen
strb r0,[r7,5]
str r2,[r7,8]
bl CopyString8x8ToVRAM

; First byte, earned items.  After that, a byte to identify each item.
ldr r6,=0x03000C54
ldrb r5,[r6]
; Move to the first item.
add r6,r6,1

; From original code.  Essentially, calculates r5 != 3 ? 1 : 0...
; This is used for the starting y position.
mov r0,3
eor r0,r5
neg r0,r0
; The sign bit, which will be set unless (r5 ^ 3) == 0.
lsr r4,r0,31

cmp r5,0
beq @spoilsDone

; Prepare the pointer to item names/info, but subtract one item since we're 1 indexed.
ldr r3,=ItemTextName - @NameLen
mov r8,r3

@nextSpoil:
; Read the item number + 1 (0 means none.)
ldrb r0,[r6]
cmp r0,0
beq @skipSpoil

; Increase y for each item.
add r4,r4,1
; Get the item name pointer, this is basically * 16.
lsl r0,r0,4
add r0,r8
str r0,[r7,12]
; Offset destination pointer by y.
lsl r1,r4,10
add r1,r9
str r1,[r7,8]

; This is the length of the item names.
mov r1,@NameLen
; This calculates length skipping trailing zeros.
bl 0x080484A8
strb r0,[r7,5]

; All ready, draw the item name.
bl CopyString8x8ToVRAM

@skipSpoil:
add r6,r6,1
sub r5,r5,1 ; Sets Z/eq if zero.
bne @nextSpoil

@spoilsDone:
; The original game draws 'found' 0x100 bytes (8 tiles) later.
; We redo this to respect the actual VWF length so there's no awkward space.
ldr r0,[r7,4]
lsr r0,r0,1
; Also add a bit for a space.
add r0,20
; Now just add that to r9, easy.
add r9,r0

; Now we draw 'found'.
ldr r0,=@FoundTextPtr
str r0,[r7,12]
mov r0,@FoundTextLen
strb r0,[r7,5]
lsl r0,r4,10
add r0,r9
str r0,[r7,8]
bl CopyString8x8ToVRAM

mov r0,2
mov r1,2
bl 0x08048584

b 0x08051028
.pool
.endregion
