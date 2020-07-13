.ifdef NameMaxLength
; When displaying save data, a small save buffer is created with only 4 bytes for name.
; In this function, we patch to put the name length there, and use another buffer.
.org 0x08075AFC
.area 0x08075BFC-.,0x0
; Args: uint8_t saveNum, uint8_t *buffer
.func SavingLoadSaveInfo
push r4-r6,r14
; Utility/font params struct.
ldr r4,=0x030018BC
; Save these for upcoming func calls.
mov r5,r0
mov r6,r1

; Load the entire savedata into slower WRAM.
strb r5,[r4,1]
bl 0x0802F34C

; This is where it was loaded - 0x02000000 + saveNum * 0x600.
; (x * 3) << 9 = x * 0x600
lsl r0,r5,1
add r0,r0,r5
lsl r0,r0,9
mov r1,2
lsl r1,r1,24
orr r0,r1

; That goes into the utility params src and we call a func to verify the data.
str r0,[r4,12]
bl 0x0802EC2C

; That return the save type, which is our first flag.
; It's either 1 or 2.
strb r0,[r6,0]

cmp r0,2
beq @@regularSave

; Empty save, clear everything out.
; Write 8 bytes of FF using memset.
add r0,r6,1
mov r1,0xFF
mov r2,8
bl 0x080F95AC

; And now 5 zeros (should really be 6, but mimicking the original code's bug.)
add r0,r6,1
add r0,8
mov r1,0x00
mov r2,8
bl 0x080F95AC
b @@done

@@regularSave:
; Put the name in our buffer.
ldr r0,=MSaveLongNames
lsl r2,r5,3
add r0,r0,r2
mov r1,0
bl CopyCharNameToBuffer
; Just write its length to r6.
strb r2,[r6,1]

; This function is a longcut for r0 = (int)(03001AB6.unk00b != 0).
bl 0x080758E8
strb r0,[r6,5]

; This just loads a byte +0x0285 into the loaded save data.
; That's the place the player was when they saved.
mov r0,r5
bl 0x080758AC
strb r0,[r6,6]

; Grab the main character's level.
ldr r0,=0x03001EDC + 0x4A
ldrb r0,[r0]
strb r0,[r6,7]

; I think this is which friend you've picked.
ldr r0,=0x030019D4 + 0xE0
ldrb r0,[r0]
mov r1,0x0F
and r0,r1
strb r0,[r6,8]

; This seems like time information, at least the last two are minutes and hours.
ldr r1,=0x030018DC
ldrb r0,[r1,6]
strb r0,[r6,9]
ldrb r0,[r1,5]
strb r0,[r6,10]
ldrb r0,[r1,4]
strb r0,[r6,11]
; Truncated, which means it'll wrap at 256 hours..
ldrh r0,[r1,2]
strb r0,[r6,12]

; Not sure what this does, exactly, related to 0x030032E7.
mov r0,0x28
str r0,[r4,4]
bl 0x0806AE2C

; Okay, now flip that - store 1 if it was 0, 0 if it wasn't.
ldrb r0,[r4,1]
mov r1,1
cmp r0,0
beq @@keepOne
mov r1,0
@@keepOne:
strb r1,[r6,13]

; And another flag, this one we store directly.
mov r0,0x2D
lsl r0,r0,4
str r0,[r4,4]
bl 0x0806AE2C

; Note: this is the byte that stays uninitialized if clear.
ldrb r0,[r4,1]
strb r0,[r6,14]

@@done:
pop r4-r6,r15
.pool
.endfunc
.endarea

; We patch each of the saving renderers to draw names in a different place.
; Generally, they load save data into a 15 byte info record with name embedded.
; We need a longer name so we put it elsewhere.
;
; Lots of duplication because they use different registers...

; This is the save screen, a small part of 0807A5F8.
.org 0x0807A768
.area 0x0807A77C-.,0x00
; Get the name and save number.
mov r0,r6
ldrb r1,[r2,1]
ldr r2,[0x0807A7F0] ; 0x06011200
lsl r3,r6,10
add r2,r2,r3
bl SavingRenderName
b 0x0807A77C
.endarea
; We update the literal pool, used to have the old draw dest.
.org 0x0807A7F0
dw 0x06011200

; This is while actually saving, a small part of 0807AC3C.
.org 0x0807AE00
.area 0x0807AE14-.,0x00
; Get the name and save number.
mov r0,r5
ldrb r1,[r0,1]
ldr r2,[0x0807AEDC] ; 0x06011200
lsl r3,r5,10
add r2,r2,r3
bl SavingRenderName
b 0x0807AE14
.endarea
; We update the literal pool, used to have the old draw dest.
.org 0x0807AEDC
dw 0x06011200

; This is the load screen, a small part of 0807B028.
.org 0x0807B150
.area 0x0807B164-.,0x00
; Get the name and save number.
mov r0,r6
ldrb r1,[r2,1]
ldr r2,[0x0807B23C] ; 0x06011200
lsl r3,r6,10
add r2,r2,r3
bl SavingRenderName
b 0x0807B164
.endarea
; We update the literal pool, used to have the old draw dest.
.org 0x0807B23C
dw 0x06011200

; This is the delete screen, a small part of 0807B4C0.
.org 0x0807B628
.area 0x0807B63C-.,0x00
; Get the name and save number.
mov r0,r6
ldrb r1,[r2,1]
ldr r2,[0x0807B6B0] ; 0x06011200
lsl r3,r6,10
add r2,r2,r3
bl SavingRenderName
b 0x0807B63C
.endarea
; We update the literal pool, used to have the old draw dest.
.org 0x0807B6B0
dw 0x06011200

; This might be while erasing (?), but I don't see it get called.  A small part of 0807BAE8.
.org 0x0807BCA8
.area 0x0807BCBC-.,0x00
; Get the name and save number.
mov r0,r5
ldrb r1,[r0,1]
ldr r2,[0x0807BD54] ; 0x06011200
lsl r3,r5,10
add r2,r2,r3
bl SavingRenderName
b 0x0807BCBC
.endarea
; We update the literal pool, used to have the old draw dest.
.org 0x0807BD54
dw 0x06011200

; We stuff this somewhere we have space...
.org SavingRenderNameLoc
.area 0x0802D420-.
; Args: uint8_t saveNum, uint8_t nameLen, void *dest
.func SavingRenderName
push r14
; Make our save name pointer.
ldr r3,=MSaveLongNames
lsl r0,r0,3
add r0,r0,r3

; Utility/font params struct.
ldr r3,=0x030018BC
strb r1,[r3,5]
str r2,[r3,8]
str r0,[r3,12]

bl CopyString8x8ToVRAM

pop r15
.pool
.endfunc
.endarea

; Alright, time for some updated sprite OAM data.
.org SaveNameSprites
.area 0x0038,0x00
; This is the "base" count and OAM for names, it adds X and Y to each.
@Name1Sprite:
dh 2
	dh 0x4000, 0x4000, 0x0090
	dh 0x4400, 0x4020, 0x0494
@Name2Sprite:
dh 2
	dh 0x4000, 0x4000, 0x00B0
	dh 0x4400, 0x4020, 0x04B4
@Name3Sprite:
dh 2
	dh 0x4000, 0x4000, 0x00D0
	dh 0x4400, 0x4020, 0x04D4
@Name4Sprite:
dh 2
	dh 0x4000, 0x4000, 0x00F0
	dh 0x4400, 0x4020, 0x04F4
.endarea

; Now overwrite the pointers to the OAM data.  Save/Load/Delete each use this.
.org 0x08456DD8
dw @Name1Sprite, @Name2Sprite, @Name3Sprite, @Name4Sprite
.endif
