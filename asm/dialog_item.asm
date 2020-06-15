; Normally when [ITEM] is used to show an obtained item's name in dialog,
; the name is stored in a fixed size buffer at 0x0300199C in 0806B27C.
; We redo this to use a different buffer.

; This is also used in battle_enemies.asm, but they never run at the same time.
@WorkArea equ MBattleEnemyWorkArea

; This touches items, clothes/equipment, and gimmicks.
; It can also show a count received, if more than one.  We change that too, since
; it used hardcoded Japanese.

; Exclusively called by 0806AD80, which is called by 0806DBD8 and 0806EBC4.
; 0806EBC4 seems to deal with its length, but doesn't read the text.
; 0806DBD8 actually reads the text we write.  We patch that too.
org 0x0806B27C
.area 0x0806B3A4-.,0x00
.func FormatObtainedItem
push r4-r7,r14
sub sp,4
; 03001AD2 holds: (0x80 | ID) for clothes, (0xC0 | ID) for gimmicks, and ID for items.
; 03001AD3 holds the count obtained.
ldr r4,=0x3001AD2
; Font drawing parameters.
ldr r5,=0x030018BC
ldr r6,=@WorkArea

; We'll track # of characters written here.
mov r7,0

; If they got more than one, we show a prefix i.e. "2x ".
; The original game used a suffix and hardcoded ko.
ldrb r0,[r4,1]
cmp r0,1
bls @@onlyOne

; This is basically itoa(), [r5,12] is set to the start and [r5,1] is the length.
; There's a 5 byte buffer at 03001ACD the game normally uses, but let's just use ours.
str r6,[r5,12]
str r0,[r5,4]
bl 0x080715A4;

ldrb r3,[r5,1]
add r7,r7,r3
ldr r2,[r5,12]

; Move it to the right place in the buffer.
@@nextDigit:
ldrb r0,[r2,0]
strb r0,[r6,0]
add r2,r2,1
add r6,r6,1
sub r3,r3,1 ; Sets Z/eq on zero.
bne @@nextDigit

; Now we add an 'x' and a space.  Original game used 0x2A (ko.)
mov r0,0x98
strb r0,[r6,0]
mov r0,0xEF
strb r0,[r6,1]
add r6,r6,2
add r7,r7,2

@@onlyOne:
; Get the name based on flags.
; We'll put ptr in r0, base in r1, and maxLen in r4.
ldrb r0,[r4,0]
mov r1,0x80
tst r0,r1
bne @@notItem

; Okay, must be an item.  Grab the name.
lsl r0,r0,4
ldr r1,=ItemTextName - 0x10
mov r4,0x10
b @@nameReady

@@notItem:
bic r0,r1
mov r1,0x40
tst r0,r1
bne @@gimmick

; In this case, it's clothing.  The bic above cleared the high bit.
lsl r0,r0,4
; This is actually the only place the name in this struct was used, and we replace it.
ldr r1,=ClothesText - 0x10
mov r4,0x10
b @@nameReady

@@gimmick:
; The game called 0x0806AD60 here, but there's no need.
lsl r0,r0,5
ldr r1,=GimmickText - 0x40
mov r4,0x10

@@nameReady:
add r0,r1

; At this point, r0 is the string and r4 the maxLen.
; Let's use the game's utility copy to our workarea.
str r0,[r5,12]
str r6,[r5,8]
str r4,[r5,4]
bl 0x0806A00C

; Almost done.  Calculate the actual name length, ignoring trailing NULs.
strb r4,[r5,1]
str r6,[r5,12]
bl 0x0806A0D8

; Restore r6 to the start of the string.
sub r6,r6,r7

; We have to write the total length to 0300198C now.
; This includes the "2x " if we added it.
ldr r3,=0x0300198C
ldrb r0,[r5,1]
add r7,r7,r0
strb r7,[r3,11]

; We're constrained in 0806DBD8, so help it by storing the pointer and total length.
str r6,[r5,12]
strb r7,[r5,1]

add sp,4
pop r4-r7
pop r0
bx r0
.pool
.endfunc
.endarea

; This is the code the handles [FF 85] or [ITEM] codes in dialog text.
; It needs to look at the workarea that 0806B27C wrote.
org 0x0806DBD8
.area 0x0806DC10-.,0x00
.func HandleDialog85Item
push r4,r14
; This is the source of our parameters.  We copy to avoid messing with the other caller.
; The original func goes through another level of indirection which gives us too many literals.
ldr r1,=0x03003A34
ldr r2,=0x03001AD2

ldrh r0,[r1,0]
strh r0,[r2,0]
; Build the item name.
bl 0x0806B27C
; Append to current position in 0x03004DDC.
; Params already set by 0806B27C.
bl 0x0806DA40
; This marks flag 1 in the dialog state.
bl 0x0806CE80

pop r4
pop r0
bx r0
.pool
.endfunc
.endarea
