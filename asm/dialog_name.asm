; When rendering dialog messages, codes 0x81 through 0x84 are the character names.
; We replace these to handle longer names.

.ifdef NameMaxLength
; The original duplicated a bunch of code for each.  They're basically the same, so we don't.
.org 0x0806DB58
.area 0x0806DBD8-.,0x00
.func HandleDialog81Name1
mov r1,0
b HandleDialog8xName
.endfunc

.func HandleDialog81Name2
mov r1,1
b HandleDialog8xName
.endfunc

.func HandleDialog81Name3
mov r1,2
b HandleDialog8xName
.endfunc

.func HandleDialog81Name4
mov r1,3
b HandleDialog8xName
.endfunc

.func HandleDialog8xName
push r14

ldr r0,=MLongCharNameWorkArea
bl CopyCharNameToBuffer

ldr r3,=0x030018BC
; Length in r2 from the above call.
strb r2,[r3,1]
str r0,[r3,12]

bl 0x806DA40
bl 0x806CE80

pop r0
bx r0
.pool
.endfunc
.endarea

; 0x08649F70 has a table of { int code; void *handler; int args; }.  Update the handlers.
.org 0x0864a040
dw org(HandleDialog81Name1) + 1
.org 0x0864a04c
dw org(HandleDialog81Name2) + 1
.org 0x0864a058
dw org(HandleDialog81Name3) + 1
.org 0x0864a064
dw org(HandleDialog81Name4) + 1
.endif
