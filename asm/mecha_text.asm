; This simply patches the game to use different addresses for the text.
; Normally, name is 0x08 bytes at i * 0x2C + 0, and description is 0x1A bytes at i * 0x2C + 8.
; Original data was at 0x08639784.

@NameLen equ 0x18
@DescLen equ 0x30
@Stride equ @NameLen + @DescLen

; 0804ACB0 shows the names.  We patch to use our longer name buffer.
.org 0x0804ACF0
db @Stride * 2
.org 0x0804AD40
dw org(MechaText)
.org 0x0804ACFC
.area 0x0804AD24-.,0x00
tst r0,r1
beq @@skipMecha

mov r0,@Stride
mul r0,r4

; This is the text base, which we patch.
ldr r1,[0x0804AD40]
add r0,r0,r1
add r0,r0,r6
mov r1,@NameLen
bl CalcFixedStringLength

; Okay, write out the corrected length and string pointer.
strb r1,[r5,5]
str r0,[r5,12]

; Destination VRAM address.
lsl r0,r4,10
ldr r1,[0x0804AD44] ; 0x06000CE0
add r0,r0,r1
str r0,[r5,8]

mov r0,14
bl CopyString8x8ClearR0

@@skipMecha:
add r4,r4,1
.endarea

; 0804FA6C shows the description in 3 places, which we update for longer text.
; Descriptions are copied to a fixed size buffer at 0x03000BFC, but we want longer.
; See battle_message.asm.
.org 0x0804FB38
dw org(MechaText) + @NameLen
.org 0x0804FAF4
mov r1,@DescLen
.org 0x0804FAFA
mov r1,@Stride * 2
; Just push the pointer.
.org 0x0804FB06
.area 0x0804FB0C-.,0x00
add r0,r0,2
str r1,[r0]
nop
.endarea

.org 0x0804FC88
dw org(MechaText) + @NameLen
.org 0x0804FBCA
mov r1,@DescLen
.org 0x0804FBD4
mov r1,@Stride * 2
.org 0x0804FBDC
mov r2,@Stride
.org 0x0804FBE6
.area 0x0804FBEC-.,0x00
add r0,r0,2
str r1,[r0]
nop
.endarea

; Reuses the same literal as above.
.org 0x0804FC2C
mov r1,@DescLen
.org 0x0804FC36
mov r1,@Stride * 2
.org 0x0804FC3E
mov r2,@Stride
.org 0x0804FC48
.area 0x0804FC4E-.,0x00
add r0,r0,2
str r1,[r0]
nop
.endarea
