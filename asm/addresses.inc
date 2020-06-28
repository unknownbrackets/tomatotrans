; This file is for "allocated" addresses just to keep track.
.definelabel BattleMenuText,0x087F0000 ; length 0x1E
.definelabel GimmickText,0x087F001E ; length 0x0C80
.definelabel ItemTextName,0x087F0C9E ; length 0x0610
.definelabel ItemTextDesc,0x087F12AE ; length 0x0330
.definelabel ClothesText,0x087F15DE ; length 0x0200
.definelabel BattleAilmentsText,0x087F17DE ; length 0x0060
.definelabel NamingScreenText,0x087F183E ; length 0x0140
.definelabel DefaultNamesText,0x087F1980 ; length 0x0020
.definelabel MechaText,0x087F19A0 ; length 0x0240
.definelabel MenuNameSprites,0x087F1BE0 ; length 0x0038
.definelabel SaveNameSprites,0x087F1C18 ; length 0x0038
; Next at 0x087F1C50.

; Some places need RAM for temporary storage.
; The game doesn't seem to use all of the WRAM in any code, so we'll use that.
MBattleEnemyWorkArea equ 0x02038000 ; length 0x30
MFontClearSize equ 0x02038030 ; length 0x2
MFontXOffset equ 0x02038032 ; length 0x2
MFontControlCodeData equ 0x02038034 ; length 0x4
MLongCharNameWorkArea equ 0x02038038 ; length 0x0008
MSaveLongNames equ 0x02038040 ; length 0x0020
