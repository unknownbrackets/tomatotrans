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

@NameOffset equ 0
@NameLen equ 0x10
@DescOffset equ @NameLen
@DescLen equ 0x30
@Stride equ @NameLen + @DescLen

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

; TODO: 0806AD60 stores the name pointer, but doesn't do length directly.
; 0806B27C, its only caller, does calculate the length but needs more investigation.

; TODO: 08065518 copies the gimmick to a buffer (0x03003F34), and then checks its length.
; After that, 0802B7F0 draws the text.  Need to find this too.

; Descriptions are copied to a fixed size buffer at 0x03000BFC, but we want longer.
; Really, they don't need to be there - we can read directly from the ROM.
; We edit the func to use a pointer in that buffer for long strings, instead.

; It's pretty long so just rewriting the relevant part.  All regs are actually free.

; Original buffer:
; uint8_t align = { LEFT = 0, CENTER = 1, RIGHT = 2 }; // Doesn't work with VWF.
; uint8_t length;
; char str[0x1E]; // We store pointer at &str[2] for alignment reasons.

; Before the literal pool...
.org 0x0804A4E0
.area 0x0804A4F8-.,0x00
; Load the existing 0x03000BFC from the literal pool.
ldr r0,[r15,0x0804A50C-.-4]

; Read length into r7 and skip on length zero.
ldrb r7,[r0,1]
cmp r7,0
beq 0x0804A54C

; Load the font draw params struct - 0x030018BC.
ldr r5,[r15,0x0804A580-.-4]

; Now the alignment flag.
ldrb r1,[r0,0]
mov r6,0
; Used for alignment, not compatible with VWF really...
; Kept to avoid breaking other strings.
mov r2,0x1A

; 0 = left, 1 = center, 2 = right.
cmp r1,1
beq @alignCenterDesc
bgt @alignRightDesc
b @alignDoneDesc
.endarea

;After the literal pool...
.org 0x0804A510
.area 0x0804A54C-.,0x00
@alignCenterDesc:
; xpos = (0x1A - length) / 2
mov r6,r2
sub r6,r7
lsr r6,r6,1
b @alignDoneDesc

@alignRightDesc:
; xpos = 0x1A - length
mov r6,r2
sub r6,r7

@alignDoneDesc:

; This is the literal pool address for 0x06000820.
ldr r1,[r15,0x0804A588-.-4]
; Set the dest parameter for drawing.
lsl r6,r6,5
add r1,r6
str r1,[r5,8]

; Now, if length is > 0x1A, read as a pointer.  r2 still has 0x1A.
cmp r7,r2
bgt @@largeText

; Use the buffer as the source for < 0x1A.
add r0,r0,2
b @@drawText

@@largeText:
ldr r0,[r0,4]

@@drawText:
; Actually store the string pointer.
str r0,[r5,12]

mov r1,r7
; This calculates the length less padding.  So we don't draw too many spaces at the end.
; Expects r0=str, r1=maxLength
bl 0x080484A8
strb r0,[r5,5]

; Draw the description.
bl 0x8071748

; There's a lot of blank space now, skip it.
b 0x0804A54C
.endarea

; Below we actually store the pointer for desc, 4 times in 2 functions.

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
