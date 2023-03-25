# [**fheroes2**](README.md) translation guide

This project uses portable object (PO) files to handle localization in various languages. The PO files are located in the `files/lang`
subdirectory of the project source tree. The current instruction is designed for Linux, MacOS and Windows users.

## Current status

![Belarusian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_be.json)
![Bulgarian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_bg.json)
![Czech](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_cs.json)
![German](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_de.json)
![Danish](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_dk.json)
![Spanish](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_es.json)
![French](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_fr.json)
![Hungarian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_hu.json)
![Italian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_it.json)
![Lithuanian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_lt.json)
![Norwegian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_nb.json)
![Dutch](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_nl.json)
![Polish](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_pl.json)
![Brazilian Portuguese](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_pt.json)
![Romanian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_ro.json)
![Russian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_ru.json)
![Slovak](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_sk.json)
![Swedish](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_sv.json)
![Turkish](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_tr.json)
![Ukrainian](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_uk.json)

## Finding translatable strings in the codebase

Translatable strings can be found in the source code as arguments to the `_` function (a short name for `gettext`),
`_n` function (a short name for `ngettext`) and `gettext_noop` function used only as indicator for the future string
translation. The string below, for example, can be translated in PO files:

```cpp
_( "Are you sure you want to quit?" )
```

## Adding new translations/localizations

If you want to add a new localization, you will first need to add it to the `SupportedLanguage` list/enumeration in the source code.
Afterwards, a new PO file for it will need to be added. It will have to be named according to the ISO standard's two-character
language abbreviations. Then to have font support, you will have to specify what font encoding/charset is compatible by adding
the language to the font generation code found in `src/fheroes2/gui/ui_font.cpp`. If a compatible font encoding has not currently
been implemented, then code for that will need to be written.

## Editing translations

We encourage you to use [**poedit**](https://poedit.net/) or [**gtranslator**](https://wiki.gnome.org/Apps/Gtranslator) to
edit translations.

Currently all implemented languages, except French, adhere to a standardized font encoding/charset.

NOTE: The fheroes2 team has set a maximum of 400 total modified lines for any Pull Request (PR) for translations. For contributors wanting to
add translated lines to a new language this has a maximum of 30 total modified lines for that first PR.

These limitations have been set because every PR needs to be reviewed by our team, and so changing too many lines at once will only slow this
process down. In addition, GitHub becomes increasingly difficult to navigate once too many changes, comments and so on are present within the
same PR page, further slowing down the process of reviewing it.

Furthermore, we have decided on a minimum amount of 15 changed strings for a translation PR. For languages that have translations that are more
or less complete, less than this amount can be accepted.

Preferrably a PR should contain a small amount of changes, about 100 lines, all focused on translating a specific part of the game - for
example creature names or castle buildings.

## Build binary translation files

Once the translation files have been modified, for Linux/MacOS run the `make` command below in the `files/lang` subdirectory to create
machine object (MO) binary files which can be used by the fheroes2 engine.

For example, for the German PO file, `de.po`, the following would be the command:
```bash
make de.mo
```

To have this MO file used by the engine, it should then be placed in the `files/lang` folder used by the fheroes2 executable.
The exact location of this folder depends on the operating system. On Windows, it is usually located in the app installation
directory. On Linux, it is usually located in the `/usr/share/fheroes2` or `/usr/local/share/fheroes2`. Currently for MacOS 
users this location is dependent on what third-party package manager is used to install fheroes2. The Flatpak version of the 
fheroes2 installation from Flathub is located in the `usr/.var/app/io.github.ihhub.Fheroes2` directory.


For Windows users who use POEdit or a similar application, it is possible to compile the MO file using such a program. However, note that
the program will need to be set to compile the MO file in the font encoding/Charset that the language that you are translating to has been
set to.

For example, for German you will have to set font encoding to CP1252, while for Russian this would be CP1251. Later when submitting
a PR with your changes, you will have to save the PO file in UTF-8 encoding because this is what Github supports.

## Updating PO templates and translatable strings in PO files

Currently all PO files are automatically updated with new strings after each commit that brings changes to the ingame text. If for whatever
reason you still need to update strings locally, this can be achieved by running the command below in `src/dist` to generate a new portable
object template (POT) file. Windows users will need to setup an environment that lets them run `make`, like Windows Subsystem for Linux (WSL)
or [Cygwin](https://www.cygwin.com/)/[MSYS2](https://www.msys2.org/).

```bash
make pot
```

Once the POT file has been created, go to the `files/lang` folder and run the command below to update translatable strings in the PO files.
If you are using programs mentioned above like POEdit, then they have options to merge new strings from a POT file.

```bash
make merge
```
