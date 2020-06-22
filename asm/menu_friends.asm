.ifdef NameMaxLength
; This is a small part of 0807C96C, which draws the Friends status content.
; Just adjusting the name length.
.org 0x0807C98E
.area 0x0807C9AC-.,0x00
; At this point: r0=FREE, r1=FREE, r2=FREE
; r4=0x03004FF8 (menu params), r5=FREE, r6=FREE, r7=FREE, r8=dest

; This is the char number.
ldrb r0,[r4,0x1F]
; Get our utility params struct.
ldr r3,[0x0807CAB8] ; 0x030018BC

mov r1,r8
; Our X/Y offset is 0x140 + 0x400.
mov r2,0x54
lsl r2,r2,4
add r1,r1,r2
str r1,[r3,8]

mov r1,6
bl CopyCharName8x8ToVRAMClearR1

b 0x0807C9AC
.pool
.endarea

; This replaces a value in the literal pool (since we no longer need the old one.)
.org 0x0807CAB8
dw 0x030018BC

; This function draws the names on the left of the Friends menu.  We fix the lengths.
.org 0x08072FBC
.area 0x08073034-.,0x00
.func FriendsMenuDrawSidebarNames
mov r3,r8
push r3-r7,r14
; This is where our dest address lives.
ldr r7,=0x03004FF8
ldr r7,[r7,0]
; Add our Y, 0x30 pixels or 6 tiles.
mov r0,6
lsl r0,r0,10
add r7,r7,r0
; And our base X, 1 tile.
add r7,0x20

; Grab the party size and start a counter in r6.
ldr r0,=0x030035F0
ldrb r0,[r0,0]
mov r8,r0
mov r6,0

; Set up the y stride for each character (except the first.)
mov r5,3
lsl r5,r5,10

; And finally the utility/drawing params struct.
ldr r4,=0x030018BC

@@nextChar:
mov r0,r6
mov r1,6
str r7,[r4,8]
bl CopyCharName8x8ToVRAMClearR1

add r7,r7,r5
add r6,r6,1

cmp r6,1
bne @@skipExtraSpacing
; Conveniently, we want 1 tile of space after the first one.
lsl r0,r6,10
add r7,r7,r0
@@skipExtraSpacing:

cmp r6,r8
blt @@nextChar

pop r3
mov r8,r3
pop r4-r7,r15
.pool
.endfunc
.endarea
.endif
