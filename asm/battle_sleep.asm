; See also battle_enemies.asm.
@AilmentStride equ 0x18

; 080502A4 reads the character name and status message for sleep.
; We rewrite because it uses a small buffer for its message.
.org 0x0804D978
.area 0x0804DAAC-.,0x00
; Note: ultimately called by 0804851C through various pointer lookups.
; Doesn't take any arguments.
.func BattleStatPlayerStatusMsg
push r4-r7,r14
; Keep aligned.
add sp,-4

; The currently active battle character #.
ldr r6,=0x03000BB6
ldrb r6,[r6,0]

; This is that character's status, stride 132/0x84.
ldr r7,=0x03000938
; (x * 32 + x) * 4 = 132
lsl r0,r6,5
add r0,r0,r6
lsl r0,r0,2
add r7,r7,r0

; This gets a flag indicating if the message is showing or not.
mov r0,2
bl 0x08048574

; Stored here is the count of frames the message has shown during.
ldr r5,=0x03000BAC

cmp r0,1
blo @@messageNotVisible
bhi @@return

; Okay, here the message is visible.  Increase the frame counter.
ldrb r4,[r5]
add r4,r4,1
cmp r4,60
blo @@keepShowing

; In this path, we're hiding it - start by resetting the counter.  We save later.
mov r4,0

; This updates the battle or message status flags, I think.
mov r0,2
bl 0x0804A148

; This is the recovery counter, at 9.  It goes up until it hits the value at 10.
; Not entirely sure why we check it here.
ldrb r0,[r7,9]
cmp r0,0
beq @@hideCounterZero

; Store FF in the character status - maybe saying inactive?
mov r1,0xFF
strb r1,[r7,1]

; Not sure what this value is, but we store it too.
ldr r1,=0x03000BF1
mov r0,1
strb r0,[r1,0]

mov r0,0
mov r1,0
b @@hideCounterNotify

@@hideCounterZero:
; I think this sets it showing, maybe in case they just recovered?
mov r0,1
mov r1,2

@@hideCounterNotify:
; This updates message status (same checked by 0x08048574.)
bl 0x08048584
; Intentional fall through to save the counter, we're done.

@@keepShowing:
strb r4,[r5]
b @@return

@@messageNotVisible:
; If the target for wakeup was FF, we set to 0 (not sure why.)
ldrb r4,[r7,10]
cmp r4,0xFF
bne @@skipTargetUpdate
; We'll save this later.
mov r4,0
@@skipTargetUpdate:

; Now check if they're actually asleep.
ldrh r1,[r7,16]
mov r2,4
tst r1,r2
beq @@bailBadStatus

; Okay, clear the frame counter.  r0 is still 0 from the blo.
strb r0,[r5]

; Let's get flag updates out of the way.  First, 0x4001.
mov r0,0x040
lsl r0,r0,8
add r0,r0,1
bl 0x0804A148
; And now the message status.
mov r0,2
mov r1,1
bl 0x08048584

; This is the character number in the party (i.e. 0=Demille.)
ldrb r1,[r7,0]
; This is the target buffer for battle messages.
ldr r0,=0x03000BFC + 2

.ifdef NameMaxLength
	; This maintains r0/r1, and updates r2 to the actual length.
	bl CopyCharNameToBuffer
.else
	; Get the character name pointer.  Multiply by 96.
	lsl r3,r1,1
	add r3,r3,r1
	lsl r3,r3,5
	ldr r1,=0x03001EDC
	add r1,r1,r3

	; Save the length for a bit later.
	ldrb r5,[r1,4]

	; r2 is still 4, let's memcpy.
	bl 0x080F954C

	; Simulate the long name path.
	ldr r0,=0x03000BFC + 2
	mov r2,r5
.endif

; Now that we have the length, update the buffer info.
sub r1,r0,2
; Also update the buffer to the end of the name.
add r0,r0,r2
mov r3,0 ; LEFT
strb r3,[r1,0]
; Message max length is 18 here (not 0x18.)  We'd need a new buffer for 0x18.
add r2,18
strb r2,[r1,1]

; Okay, this is the base message - " is asleep".
ldr r1,=BattleAilmentsText
mov r2,18

; Now load the sleep counter.  This is how long I've slept, so we increase by one too.
ldrb r5,[r7,9]
add r5,r5,1
; The original code masked this, let's do it for safety.
lsl r5,r5,24
lsr r5,r5,24

; r4 still has our target value - when we'll wake up.
cmp r5,r4
beq @@recovery

; Save the updated target (might've changed to 0) and sleep counter.
strb r5,[r7,9]
strb r4,[r7,10]

; Keep sleeping.  r0, r1, and r2 already setup, memcpy time.
bl 0x080F954C
b @@return

@@recovery:
; Move to the woke up message and memcpy.
add r1,@AilmentStride * 2
bl 0x080F954C

; Now we update counters.
mov r1,0
strb r1,[r7,9]
strb r1,[r7,10]

; And clear the sleep status flag.
ldrh r2,[r7,16]
mov r0,4
bic r2,r0
strh r2,[r7,16]

; I'm not sure what this func does, maybe it prepares the battle menu?
mov r0,r6
; We want -1, and we have 0 in r1.
sub r1,r1,1
bl 0x0804B758

@@return:
add sp,4
pop r4-r7
pop r0
bx r0

@@bailBadStatus:
; Store the sleep target in case we changed to 0.
strb r4,[r7,10]

; Update message flags.
mov r0,1
mov r1,2
bl 0x08048584
b @@return
.pool
.endfunc
.endarea
