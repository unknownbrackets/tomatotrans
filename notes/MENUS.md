Menu insertion types
===========

There are several types of insertions supported by the inserter in the ta_menus file.

### BLOCKFIXED

Format: BLOCKFIXED PointerLoc NewLen Count [ClearLen]

This references a game text block, which is a pointer to a list of strings and sizing information.
Each string has the same fixed length.

In many cases, the length can be increased.  ClearLen is used to limit the tiles overwritten by
the game's drawing functions, and defaults to the original text width.

### FIXEDPOINTER

Format: FIXEDPOINTER PointerLoc MaxLen

This is a simple pointer with a fixed length.

### DYNPOINTER

Format: DYNPOINTER PointerLoc MaxLen SizeLoc [SizeLoc2]

This is a pointer that allows a more dynamic size.  Bytes at SizeLoc and SizeLoc2 are updated with
the new length.

### RESTRUCT

Format: RESTRUCT OldAddress OldSize OldStride NewAddress NewSize NewStride Count

This resizes an existing structure with embedded text to a new size, but only copies the text
fields.

### POINTERLIST

Format: POINTERLIST PointerLoc Stride MaxLen Count

This is essentially for a list of FIXEDPOINTER that are evenly spaced and have the same length.

### BLOCKSTART

Format: BLOCKSTART TextLoc Len Count

A simple in-place insertion.  Does not allow longer text than original.

### TERMSTRING

Format: TERMSTRING PointerLoc TermChar MaxLen

This is a string terminated by a character, i.e. FF.  The length is also the number of tiles to
replace when drawing the string, so be careful.
