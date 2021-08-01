.gba
.open "ta.gba","test.gba",0x08000000

.include "asm/addresses.inc"

; Game bugfix for saving in no$gba.  Possibly an emulator bug.
.include "asm/bug_nocash.asm"

; Required for the menu VWF fixes below to work.
.include "asm/fontmap.asm"
.include "asm/vwf_8x8.asm"

; The below lengthen text primarily in battle, and require battle_message.asm.
.include "asm/battle_message.asm"
.include "asm/battle_enemies.asm"
.include "asm/battle_sleep.asm"
.include "asm/gimmick_text.asm"
.include "asm/item_text.asm"
.include "asm/mecha_text.asm"

; Allow longer text in the main battle menu.
.include "asm/battle_menu.asm"

; Add space for Strength in level up messages.
.include "asm/battle_levelup.asm"
; Line up better when gaining items from battle.
.include "asm/battle_spoils.asm"

; Updates for the menu options and descriptions.
.include "asm/menu.asm"
.include "asm/menu_gimmicks.asm"
.include "asm/menu_friends.asm"
.include "asm/shop_messages.asm"
.include "asm/menu_items.asm"
.include "asm/menu_equipment.asm"

; Longer names in dialog message control codes and VWF support.
.include "asm/dialog_item.asm"
.include "asm/dialog_center.asm"

; Longer text and names in Gimica.
.include "asm/gimica_text.asm"
.include "asm/gimica_tutorial.asm"

; Use English uppercase/lowercase in the naming screen.
.include "asm/naming_screen.asm"

; Allow longer charcter names than the default 4.  Some in other files too.
; Comment this out to disable.
.include "asm/name_long.asm"
.include "asm/dialog_name.asm"
.include "asm/saving_screen.asm"

; Adjust the buttons on the title screen.
.include "asm/title_screen.asm"

; Adjust the place names on the overworld map.
.include "asm/map_text.asm"

; Adjust buttons and replace text drawing routine.
.include "asm/credits.asm"

; English text makes the dialog speed seems slower, so increase.
.include "asm/dialog_speed.asm"

.close
