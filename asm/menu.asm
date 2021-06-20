org 0x08078038 :: db 0xB0          // move menu option text left 1 block

.ifdef NameMaxLength
; Originally, 4 tiles were given to each name.  We need 8 tiles (2 wide sprites.)
; Nothing seems to use the space after these tiles - so we just increase the stride.
.org 0x08072F70
.region 0x08072FBC-.,0x00
.func MenuDrawNameSprites
push r4-r7,r14
sub sp,4

mov r0,0
str r0,[sp,0]

; Grab the font and utility params.
ldr r6,=0x030018BC
; Original code used tiles 0x01 - 0x10, but we need double and can't use tile 0x00.
; It looks like tiles 0x240 - 0x25F are unused in the menu, so we take them.
ldr r5,=0x06014800
mov r4,0

; This is the party size.
ldr r7,=0x030035F0
ldrb r7,[r7,0]

@@nextChar:
str r5,[r6,8]
mov r0,r4
bl CopyCharName8x8ToVRAM

; Move forward and clear, for transparency reasons.
add r5,NameMaxTiles * 32
mov r0,sp
mov r1,r5
mov r2,1
lsl r2,r2,24
add r2,(8 - NameMaxTiles) * 8
; CpuFastSet the extra tiles to 0.
swi 12

add r5,0x100 - NameMaxTiles * 32

add r4,r4,1
cmp r4,r7
blo @@nextChar

add sp,4
pop r4-r7,r15
.pool
.endfunc
.endregion

; Alright, time for some updated sprite OAM data.
.autoregion
; This is the "base" count and OAM for names, it adds X and Y to each.
@Name1Sprite:
dh 2
	dh 0x4000, 0x4000, 0x0240
	dh 0x4000, 0x4020, 0x2244
@Name2Sprite:
dh 2
	dh 0x4000, 0x4000, 0x0248
	dh 0x4000, 0x4020, 0x224C
@Name3Sprite:
dh 2
	dh 0x4000, 0x4000, 0x0250
	dh 0x4000, 0x4020, 0x2254
@Name4Sprite:
dh 2
	dh 0x4000, 0x4000, 0x0258
	dh 0x4000, 0x4020, 0x225C
.endautoregion

; Now overwrite the pointers to the OAM data.  All the menus use this.
.org 0x08456D60
dw @Name1Sprite, @Name2Sprite, @Name3Sprite, @Name4Sprite
.endif
