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

; 0806AD60 is used to draw text in dialogs, but is handled in dialog_item.asm.
; That's called when [ITEM] is used in dialog text.

; TODO: 08065518 copies the gimmick to a buffer (0x03003F34), and then checks its length.
; After that, 0802B7F0 draws the text.  Need to find this too.

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
