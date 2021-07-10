; 08074D28 originally replaced certain strings with character names.
; We use control codes to do that now, which is cleaner and supports long text better.
.org 0x08074D6C
.region 0x08074DAE-.,0x00
b 0x08074DAE
.endregion

; Essentially the same as the above in 08074CAC, this is used with the ??????? text allowed too.
.org 0x08074CC8
.region 0x08074D0A-.,0x00
b 0x08074D0A
.endregion

; Tell drawing to center the card descriptions.
.org 0x084906A7
db 0x01
; The re-arrange card descriptions too.
.org 0x0849067F
db 0x01
; Also the Gimica hub place description.
.org 0x08490627
db 0x01

; 08091934 reuses size information from another text block.
; We want to use our inserted string size.
.org 0x08091B50
dw 0x0849F62C
.org 0x08091B32
.region 0x08091B46-.,0x00
; Grab the text block.
ldr r2,[0x08091B50] ; 0x0849F62C
ldr r3,[r2,0]
ldr r2,[r2,4]
str r2,[sp]

; And now the index.
lsl r0,r5,24
asr r0,r0,24

; Dest in VRAM.
ldr r1,[0x08091B58] ; 0x0600C020

mov r2,0
.endregion
