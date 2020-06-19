; Immediately after the naming screen, this game accidentally writes to ROM memory.
; It's unclear if this should be ignored or actually trigger EEPROM behavior.
;
; In no$gba, it triggers no$gba to queue some bits which ultimately causes a crash
; when attempting to save at any point in the game.
;
; In each case, a 16-bit value is stored, and then read back as 32-bit.  The extra
; bits cause the invalid write.  The original value is saved/restored around this,
; so it only writes to the originally intended place by making it a 32-bit store.

.org 0x08065AB4
str r0,[r4,4]
.org 0x08065B4C
str r0,[r4,4]
