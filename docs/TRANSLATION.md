# [**fheroes2**](README.md) translation guide

This project uses portable object (PO) files to handle localization in various languages. The current instruction is designed for Linux/MacOS users. Windows users should install [Cygwin](https://www.cygwin.com/) in order to be able to work with translations.

## Finding translatable strings in the codebase

Translatable strings can be found in the source code as arguments to the `_` function (a short name for `gettext`), `_n` function (a short name for `ngettext`) and `gettext_noop` function used only as indicator for the future string translation. The string below, for example, can be translated in PO files:

```cpp
_( "Are you sure you want to quit?" )
```

## Updating PO templates and translatable strings in PO files

If translatable strings have been added to or removed from the code source, run the command below in `/src/dist` to generate a new portable object template (POT) file:

```bash
make pot
```

Once the POT file has been created, go to `/files/lang` and run the following command to update translatable strings in the PO files:

```bash
make *.po clean
```

## Editing translations

The PO files are located in `/files/lang`. We encourage you to use [**poedit**](https://poedit.net/) or [**gtranslator**](https://wiki.gnome.org/Apps/Gtranslator) to edit translations.

## Build binary translation files

Once the translation files have been modified, run the `make` command below in `/files/lang` to create machine object (MO) binary files which can be used by fheroes2 engine.
