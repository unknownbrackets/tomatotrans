Description
===========

Tools and hacks to allow fan translation and localization of the Game Boy Advance game, Tomato
Adventure.


Basic usage
===========

Beware: these are tools to translate the game, this is not a translation.  Usage is meant for
translators.

1. Download or clone this repo into a new folder.
2. Place the image of the game in the same folder, named `ta.gba`.
3. Modify the txt files found in the folder.
4. Run the `i.bat` command on Windows.
5. Review the test.gba image outputted.


Features
===========

As of writing, some of these features are still in progress.

 * Variable width font
 * Dynamic or increased lengths for names, descriptions, and other text.
 * Inserter for text and pointers

Missing features can be found in issues on GitHub.


Translation files
===========

Text is grouped into multiple files.  As of writing, files have an eng and jpn version.  The jpn
files can be used as referenced but are not used by the inserter.

### ta_script_eng.txt

This contains the main dialog text, which is shown using an 8x12 font.  A scripting language
triggers these messages, but they are given IDs to facilitate translation.

Note that some messages are reused (for example, when getting a Gimmick.)  Some messages are
unused in the game.

See [SCRIPT.md] for the codes used in this file.

### ta_enemies_eng.txt

This contains both enemy names and their attack names.  Both are limited to 24 characters.

These names are shown during battle using an 8x8 font.

### ta_battle_eng.txt

This contains messages shown during battle based on the scripting for enemies.  They display
using an 8x8 font.

Some strings may be reused by multiple enemies.

### ta_menus_eng.txt

This contains menu text and related messages shown in:

 * The primary menu
 * Battle and battle menus
 * Shops
 * Naming screen
 * The Gimica sub game

This text is all shown using the 8x8 font.  See [MENUS.md] for format details.


### Mock insertion

To force all known strings to be inserted, use --force, as in any of:

```
i.bat --force
armips tomatoadv.asm && a --force
```

This will replace all Japanese characters with an ID followed by As.  The ID can be used to find
the translation within files the inserter uses.


### Emulator & hardware

The changes are intended to work on common emulators and on hardware, but have not been tested
everywhere.  Contributions are welcome to improve support.

A patch is included that fixes a game bug which breaks saving in [no$gba][].


Platforms
===========

These tools were designed to be portable, but a compiler is required on platforms other than
Windows.  Steps to complete before the basic usage:

1. Install or compile [armips][].
2. Compile ta_insert.cpp, similar to: `clang++ -stdlib=libc++ -Wall ta_insert.cpp -o a`
3. Use `armips tomatoadv.asm && ./a` instead of `i.bat`.


Credits and licensing
===========

 * [AlphaDream][] for creating Tomato Adventure.
 * Kingcom (and team) for [armips][], licensed under MIT.
 * Martin Korth for [no$gba][] and its debugger.
 * Tomato for the [initial proof of concept][], permitted for use under ISC.
 * Samda Knowe, White Reflection, and El Jefe for the initial idea.
 * otakuto for [TomatoTool][], licensed under MIT.


[armips]: https://github.com/Kingcom/armips
[AlphaDream]: https://en.wikipedia.org/wiki/AlphaDream
[initial proof of concept]: https://legendsoflocalization.com/tomato-adventure/
[TomatoTool]: https://github.com/otakuto/TomatoTool
[no$gba]: https://problemkaputt.de/gba.htm
[SCRIPT.md]: notes/SCRIPT.md
[MENUS.md]: notes/MENUS.md
