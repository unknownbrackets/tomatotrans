; This simply patches the game to use different addresses for the text.
; Normally, name is 0x08 bytes at i * 0x58 + 0, and description is 0x1A bytes at i * 0x58 + 8.
; Original data was at 0x0862C250.

; Original struct guesses:
; char name[8];
; char desc[26];
; uint8_t atk;
; uint8_t maxBattery;
; uint8_t unk24;
; uint8_t uses;
; uint8_t unk26;
; uint8_t flags;
; struct { uint8_t d0; uint8_t d1; } levels[9];
; uint8_t unk3a;
; uint8_t unk3b;
; void *unk3c;
; void *somePalette;
; void *someImage;
; void **unk48;
; void **unk4c;
; void *unk50;
; void *unk54;

; These values are assumed the same in dialog_item.asm.
@NameLen equ 0x10
@DescLen equ 0x30
@Stride equ (@NameLen + @DescLen)

; In general, the code will ldr a base (maybe offset) pointer from the literal pool,
; then mov the stride in and multiply.  Near that, it will mov the length in and store it.
; We change either the literal pool or mov instructions.

; 0804AA5C reads the name of the Gimmick, and draws directly (no buffer.)
.org 0x0804AB7C
dw org(GimmickText)
.org 0x0804AAC4
db @Stride
.org 0x0804AAD0
db @NameLen
; We need to force it only to clear 8, though.
.org 0x0804AADC
bl CopyString8x8Clear8

; 0806AD60 is used to draw text in dialogs, but is handled in dialog_item.asm.
; That's called when [ITEM] is used in dialog text.

; 0802B7F0 uses a buffer copied by 08065518 into 0x03003F34.  It's too small for our name.
; We repoint it to use the original text address.
.org 0x0802B868
.area 0x0802B882-.,0x00
; For some reason the game does a needless DMA3 transfer here.  Good news, it gives us space.
; At this point: r0=gimmickBuffer, r1=FREE, r2=vramDest, r3=FREE, r4=FREE, r5=fontInfo

; Dereference the name.
ldr r1,[r0,0]
str r1,[r5,12]

add r0,0x68
ldrb r0,[r0,0]
strb r0,[r5,5]
str r2,[r5,8]

; This draws the string but checks for length=FF.
bl 0x0802BEF0
b 0x0802B882
.endarea

; Now for the function that actually loads the buffer, 08065518.
; We rewrite a good chunk of the beginning to make space...
.org 0x08065518
.area 0x0806556C-.,0x00
mov r3,r8
push r3-r7,r14

; Font util parameter struct, grab it from the existing literal pool.
ldr r7,[0x080655D4] ; 0x030018BC
; Address of the buffer that stores the gimmick info.
ldr r6,[0x080655E0] ; 0x03003F34
; Load the parameter location for the gimmick number.
ldr r4,[0x080655D8] ; 0x03001AD2

; Grab the gimmick number (offset by 1.)
ldrb r0,[r4]

; This is our offset to the text in our new buffer.
lsl r5,r0,5
; This is the copy size, multiply to calculate.
mov r2,0x58
mul r0,r2
; Load the original buffer pointer with actual data, offset because r0 is 1 indexed.
ldr r1,[0x080655DC] ; 0x0862C250 - 0x58
add r1,r1,r0
; Okay, memcpy to the buffer.  We use library memcpy instead of game util, less setup.
mov r0,r6
bl 0x080F954C

; The game uses this zero later...
mov r0,0
mov r8,r0

; Store the actual text pointer.
ldr r0,=GimmickText - @Stride
add r0,r0,r5
str r0,[r6]

; Calculate the length ignoring trailing spaces.
str r0,[r7,12]
mov r0,@NameLen
strb r0,[r7,1]
bl 0x0806A0D8
ldrb r1,[r7,1]
mov r3,r6
add r3,0x68
strb r1,[r3,0]

; Load the Uses count at 0x25.
mov r5,r6
add r5,0x25
ldrb r2,[r5]
mov r0,0x7F
mov r12,r0
and r0,r2
; r3 is still buffer+0x68, and we want 0x70.
strb r0,[r3,8]
; Now shift and store at buffer+0x6F... for some reason.
lsl r2,r2,1
strb r2,[r3,7]

; r5 is still buffer+0x25, and we want 0x27.
add r0,r5,2
ldr r1,[r0]
b 0x0806556C
.pool
.endarea

; Fix the epilog because we trimmed the prolog a bit.
.org 0x080656B8
.area 0x080656C2-.,0x00
pop r3-r7
mov r8,r3
pop r0
bx r0
.endarea

; Descriptions are copied to a fixed size buffer at 0x03000BFC, but we want longer.
; Below we actually store the pointer for desc, 4 times in 2 functions.
; See battle_message.asm.

; 0804E724 normally reads into the buffer (a few times), which we'll replace.
.org 0x0804DE4C
dw org(GimmickText) + @NameLen
.org 0x0804DE2A
db @Stride
.org 0x0804DE24
db @DescLen
.org 0x0804DE32
; Replace the memcpy with a store of the pointer.
.area 0x0804DE38-.,0x00
; r0 was the 0x03000BFC pointer + 2, add another 2 for the pointer loc.
add r0,r0,2
str r1,[r0,0]
nop
.endarea

.org 0x0804DF80
dw org(GimmickText) + @NameLen
.org 0x0804DF3E
db @Stride
.org 0x0804DF38
db @DescLen
.org 0x0804DF46
; Replace the memcpy with a store of the pointer.
.area 0x0804DF4C-.,0x00
; r0 was the 0x03000BFC pointer + 2, add another 2 for the pointer loc.
add r0,r0,2
str r1,[r0,0]
nop
.endarea

.org 0x0804E04C
dw org(GimmickText) + @NameLen
.org 0x0804DFC2
db @Stride
.org 0x0804DFBC
db @DescLen
.org 0x0804DFCA
; Replace the memcpy with a store of the pointer.
.area 0x0804DFD0-.,0x00
; r0 was the 0x03000BFC pointer + 2, add another 2 for the pointer loc.
add r0,r0,2
str r1,[r0,0]
nop
.endarea

; 0804E724 normally reads into the buffer, which we'll replace as well.
.org 0x0804E7F8
dw org(GimmickText) + @NameLen
.org 0x0804E7C8
db @Stride
.org 0x0804E7C2
db @DescLen
.org 0x0804E7D0
; Replace the memcpy with a store of the pointer.
.area 0x0804E7D6-.,0x00
; r0 was the 0x03000BFC pointer + 2, add another 2 for the pointer loc.
add r0,r0,2
str r1,[r0,0]
nop
.endarea
