; When a battle starts, the scenario is read into 030008EC.
; This has pointers to each enemy record, and its default fixed size name.
; Next, when enemies attack the attack is copied to 03000c40.
;
; Names are displayed as a message (see battle_message.asm), and the buffer
; they are copied to at 0x03000BFC has at least 0x1A of space.
; We just store a pointer instead of the name, and copy that to the buffer.
; Sometimes we have to do something more complex when there's other text added.
;
; We also need to build an ailment message like "Nutter is asleep".
; The game doesn't seem to use all of the WRAM in any code, so we'll use that.
; This is also used by dialog_item.asm but they never run at the same time.
; Make sure not to mess with it during battle.
@WorkArea equ 0x02038000 ; length 0x30

; Ailments in order: asleep, paralyzed, woke up, no longer paralyzed.
@AilmentStride equ 0x18

; The below replacements all replace a strnlen_rev check, which we replace by
; having the size straight away.  This gives us space to load a longer name.
.macro LongerNameToBufferAt,bufferAddrAddr
; In all cases, r0=nameFieldPtr, and r1-r4 are free.
; We replace the 8 byte name with a 4 byte pointer and 1 byte length.
ldrb r2,[r0,4]
ldr r1,[r0,0]

; Okay, setup the buffer now.
ldr r0,[bufferAddrAddr] ; 0x03000BFC
mov r3,1 ; CENTER
strb r3,[r0,0]
strb r2,[r0,1]
add r0,r0,2
; memcpy - we put the arguments in the right places, above.
bl 0x080F954C
.endmacro

; 0804E138 has 3 reads of the name.  Each checks the length before adding it,
; but drawing that buffer checks the length anyway.  This gives us space.
; They're each the same...
.org 0x0804E20C
.area 0x0804E22E-.,0x00
LongerNameToBufferAt 0x0804E244
b 0x0804E22E
.endarea

.org 0x0804E31A
.area 0x0804E33C-.,0x00
LongerNameToBufferAt 0x0804E378
b 0x0804E33C
.endarea

.org 0x0804E3AE
.area 0x0804E3D0-.,0x00
LongerNameToBufferAt 0x0804E42C
b 0x0804E3D0
.endarea

; 0804FC90 is for Mecha Mecha Panic, I think?  We replace the same,
; and again it has 3 roughly identical reads.
.org 0x0804FD7C
.area 0x0804FD9C-.,0x00
LongerNameToBufferAt 0x0804FDB4
b 0x0804FD9C
.endarea

.org 0x0804FE96
.area 0x0804FEB8-.,0x00
LongerNameToBufferAt 0x0804FEF4
b 0x0804FEB8
.endarea

.org 0x0804FF2A
.area 0x0804FF4C-.,0x00
LongerNameToBufferAt 0x0804FFA0
b 0x0804FF4C
.endarea

; TODO: Attack names.

; 080502A4 is more complex, because it appends status strings.
; We rewrite the whole thing because we need to change the buffer handling
; For longer strings to work.
.org 0x080502A4
.area 0x0805040C-.,0x00
; Note: ultimately called by 0804851C through various pointer lookups.
; Doesn't take any arguments.
.func BattleStatEnemyStatusMsg
push r4-r7,r14

; Grab the currently active enemy number.
ldr r6,=0x03000BB7
ldrb r6,[r6,0]

; We'll also need the enemy's status, each record is 56 so we shift-multiply.
; (r6 * 8 - r6) * 8 = r6 * 56
lsl r1,r6,3
sub r1,r1,r6
lsl r1,r1,3
ldr r7,=0x03000A40
add r7,r7,r1

; Stored here is the count of frames the message has shown during.
ldr r5,=0x03000BAC

; This tells us if the message is showing or not in r0.
mov r0,2
bl 0x08048574

cmp r0,1
blo @@messageNotVisible
bhi @@return

; Okay, here the message is visible.  Increase the frame counter.
ldrb r4,[r5]
add r4,r4,1
cmp r4,60
blo @@keepShowing

; Time to kill the message.
mov r4,0

; This updates the battle or message status flags, I think.
mov r0,2
bl 0x0804A148

; This is the recovery counter, at 04.  It goes up until it hits 05.
; Not entirely sure why we check it here.
ldrb r0,[r7,4]
cmp r0,0
beq @@hideCounterZero

mov r0,0
mov r1,0
b @@hideCounterNotify

@@hideCounterZero:
; I think this sets it showing, maybe in case they just recovered?
mov r0,1
mov r1,1

@@hideCounterNotify:
; This updates message status (same checked by 0x08048574.)
bl 0x08048584
; Intentional fall through to save the counter, we're done.

@@keepShowing:
; Don't forget to save the counter back.  We're done.
strb r4,[r5]
b @@return

@@messageNotVisible:
; Check if we should show the message.  I think flag 4 indicates if the enemy is disabled.
; We keep this for later, we may update this field.
ldrh r4,[r7,10]
; Also helps to have a 4 we'll reuse in a bit.
mov r0,4
tst r4,r0
beq @@bailBadStatus

; Reset the counter to 0.  That frees up r5.
mov r1,0
strb r1,[r5]

; Let's get the buffer in r5 now.
ldr r5,=0x03000BFC
; Set the alignment since we have a 0 prepared.
strb r1,[r5,0]

; Let's get the flags taken care of.  This sets to 0x4001.
; Conveniently had 4 there before.
lsl r0,r0,12
add r0,r0,1
bl 0x0804A148
; And also update the message status itself.
mov r0,2
mov r1,1
bl 0x08048584

; Okay, now let's grab the enemy record itself, with its name pointer.
ldr r0,=0x030008EC
lsl r6,r6,2
ldr r6,[r0,r6]
; Before, r6 was a fixed-size buffer with the name.  Now it's the name and length.
ldr r1,[r6,0]
ldrb r2,[r6,4]

; This is where we actually store the longer message.
ldr r0,=@WorkArea

; Let's get the buffer out of the way since we're about to call a func and lose regs.
; We increase workarea into r6 (for the message after the name) and store length.
add r6,r0,r2
; We'll make each message fixed size at @AilmentStride + enemy name.
mov r3,@AilmentStride
add r3,r3,r2
strb r3,[r5,1]
str r0,[r5,4]

; Okay, copy the enemy name to the buffer.
bl 0x080F954C

; Are we dealing with paralysis or sleep?
ldr r3,=0x03001978
ldr r3,[r3,0]
ldrb r3,[r3,0]
; We'll check it against this later, just preparing for the tst r3,r2.
; This bit being set indicates paralysis.
mov r2,0x40

; Here's where our messages start, we'll add to get to the target.
; Using r1 because a memcpy is coming up.
ldr r1,=EnemyAilmentsText

; Now let's increase the recovery counter and check if recovered yet.
ldrb r0,[r7,4]
add r0,r0,1
; Original game masked.  Probably wasteful, but can't tell if it could be FF.
lsl r0,r0,24
lsr r0,r0,24

; This is the target value we recover at.
ldrb r5,[r7,5]
cmp r0,r5
beq @@recovery

; Didn't recover, store the increased counter.
strb r0,[r7,4]

tst r2,r3
; First message is already "asleep", so we're done if the bit isn't set.
beq @@appendMessage

; Otherwise, it's the next one.
add r1,@AilmentStride
b @@appendMessage

@@recovery:
; We clear the 4 bit we checked before.  We still have the old status value, reuse.
mov r0,4
bic r4,r0
strh r4,[r7,10]
; Now reset the recovery counter and target.
mov r0,0
strb r0,[r7,4]
strb r0,[r7,5]

; Move forward to the "no longer" messages.
add r1,@AilmentStride * 2
tst r2,r3
; We're done if it's not set.
beq @@appendMessage

; Otherwise, we just need one more stride.
add r1,@AilmentStride

@@appendMessage:
; Okay, time to memcpy that in.
mov r2,@AilmentStride
mov r0,r6
bl 0x080F954C

@@return:
pop r4-r7
pop r0
bx r0

@@bailBadStatus:
; Update the battle or message status flags (I think?)
mov r0,0x90
bl 0x0804A148
; And also update the message status itself.
mov r0,1
mov r1,1
bl 0x08048584
b @@return
.pool
.endfunc
.endarea
