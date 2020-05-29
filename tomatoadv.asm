arch gba.thumb


org $8078038; db $0B0         // move menu option text left 1 block
org $8490470; dd $87F0000    // repoint item option text block pointer
org $8490474; db $07          // make each item option text 7 letters long

// make a blank space in the fonts, comment out to make the little tsu return in the Japanese font
org $8649636; db $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
org $864453A; db $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00