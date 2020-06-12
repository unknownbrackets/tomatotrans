.gba
.open "ta.gba","test.gba",0x08000000

.include "asm/addresses.inc"

.include "asm/fontmap.asm"
.include "asm/vwf_8x8.asm"

.include "asm/menu.asm"
.include "asm/menu_gimmicks.asm"
.include "asm/battle_menu.asm"
.include "asm/battle_message.asm"
.include "asm/battle_levelup.asm"
.include "asm/battle_spoils.asm"
.include "asm/dialog_item.asm"
.include "asm/gimmick_text.asm"
.include "asm/item_text.asm"

.close
