; 08074D28 originally replaced certain strings with character names.
; We use control codes to do that now, which is cleaner and supports long text better.
.org 0x08074D6C
.area 0x08074DAE-.,0x00
b 0x08074DAE
.endarea

; Essentially the same as the above, this is used with the ??????? text allowed too.
.org 0x08074CC8
.area 0x08074D0A-.,0x00
b 0x08074D0A
.endarea
