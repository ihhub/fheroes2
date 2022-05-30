# [**fheroes2**](README.md) translation guide

This project uses portable object (PO) files to handle localization in various languages. The PO files are located in `/files/lang`. The current instruction is designed for Linux, MacOS and Windows users.

## Finding translatable strings in the codebase

Translatable strings can be found in the source code as arguments to the `_` function (a short name for `gettext`), `_n` function (a short name for `ngettext`) and `gettext_noop` function used only as indicator for the future string translation. The string below, for example, can be translated in PO files:

```cpp
_( "Are you sure you want to quit?" )
```

## Adding new translations/localizations

If you want to add a new language localization, you will first need to add this language in the source code as a supported language. Afterwards, a new PO file for that language will need to be added. It will have to be named according to the ISO standard's two-character language abbreviations.
To have font support for your language, you will have to specify what font encoding/charset your language uses by adding it to the font generation code found in `src/fheroes2/gui/ui_font.cpp`. If a font supporting your language has not currently been implemented then code for that will need to be added.

## Editing translations

We encourage you to use [**poedit**](https://poedit.net/) or [**gtranslator**](https://wiki.gnome.org/Apps/Gtranslator) to edit translations.

Currently all implemented languages, except French, adhere to a standardized font encoding/charset.

## Build binary translation files

Once the translation files have been modified, for Linux/MacOS run the `make` command below in `/files/lang` to create machine object (MO) binary files which can be used by fheroes2 engine.

For the German de.po, this would be the command:

```bash
make de.mo
```

For Windows users that use POEdit or similar, they can compile with that program. However, note that the program will need to be set to compile the MO file in the font encoding/Charset that the language that you are translating to has been set to.

For example, for German you will have to set font encoding to CP1252, and for Russian this would be CP1250. Later when submitting a PR with your changes, you will have to save the PO file in UTF-8 encoding, because this is what Github supports.

## Updating PO templates and translatable strings in PO files

Currently all PO files are automatically updated with new strings after each commit that brings changes to the ingame text. Should you still need to update strings locally, this can be acheived by running the command below in `/src/dist` to generate a new portable object template (POT) file. Windows users will need to setup an environment that lets them run 'make', like Windows Subsystem for Linux (WSL) or [Cygwin](https://www.cygwin.com/).

```bash
make pot
```

Once the POT file has been created, go to `/files/lang` and run the command below to update translatable strings in the PO files. If you are using programs mentioned above like POEdit, then they have options to merge new strings from a POT file.

```bash
make merge
```

Alternatively, to update a specific language you can run the following command, using the French PO file `fr.po` as an example:

```bash
make fr.po
```
