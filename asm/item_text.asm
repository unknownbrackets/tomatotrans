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
