.gba
.open "ta.gba","test.gba",0x08000000

.include "asm/addresses.inc"

.include "asm/fontmap.asm"
.include "asm/vwf_8x8.asm"

.include "asm/menu.asm"
.include "asm/battle_menu.asm"

.close
