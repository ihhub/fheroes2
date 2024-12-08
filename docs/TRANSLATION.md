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
![Vietnamese](https://img.shields.io/endpoint?url=https://ihhub.github.io/fheroes2/json/lang_vi.json)


## Adding new translations/localizations

If you want to add a new localization, you will first need to add it to the `SupportedLanguage` list/enumeration in the source code.
Afterwards, a new PO file for it will need to be added. It will have to be named according to the ISO standard's two-character
language abbreviations. Then to have font support, you will have to specify what font encoding/charset is compatible by adding
the language to the font generation code found in `src/fheroes2/gui/ui_font.cpp`. If a compatible font encoding has not currently
been implemented, then code for that will need to be written.


## Editing translations - Before you start

Before starting, you will have to set up the necessary environment: fork the fheroes2 repository and install an application used 
for translating. When you have your own fork of fheroes2 you should create a branch. At this point you can start translating. 
After you finish your work, and you are ready to share it, you will have to prepare a pull request.


### Forking the fheroes2 repository

A fork is your copy of the fheroes2 repository. It gives you a safe environment to prepare and test your changes.
You can read here how to [**create a fork**](https://docs.github.com/en/get-started/quickstart/fork-a-repo).


### Translation editing software

We encourage you to use [**poedit**](https://poedit.net/) or [**gtranslator**](https://wiki.gnome.org/Apps/Gtranslator) to
edit translations. Currently all implemented languages adhere to a standardized font encoding/charset.


## Editing translations - For the first time or again

### Syncing your fork

Before you start working on the first or next translation, make sure that your fork has all the recent changes from the fheroes2 repo.
To do this, [**sync your fork**](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/syncing-a-fork).

### Creating a branch

Next you will set up an environment for working on your translation, in addition to testing and sharing your changes.
Read more [**about branches**](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/about-branches).

To create a branch, please follow GitHub's instructions on
[**creating a branch within your repo**](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-and-deleting-branches-within-your-repository).

### Submitting translations

Finally you will use your translation software of choice to make changes to the translation. All original translatable strings are
located in the source code of the project. If you need to clarify where this string is being used you can search for it.
The string below, for example, can be translated in PO files:

```cpp
_( "Are you sure you want to quit?" )
```

### Testing your changes

Once the translation files have been modified, for Linux/MacOS run the `make` command below in the `files/lang` subdirectory to create
machine object (MO) binary files which can be used by the fheroes2 engine.

For example, for the German PO file, `de.po`, the following would be the command:
```bash
make de.mo
```

To make the engine use this MO, the file should be placed in the `files/lang` folder used by the fheroes2 executable.
The exact location of this folder depends on the operating system.

On Windows, it is usually located in the app installation directory.

On Linux, it is usually located in the `/usr/share/fheroes2` or `/usr/local/share/fheroes2`.

Currently for MacOS users this location is dependent on what third-party package manager is used to install fheroes2.

The Flatpak version of the fheroes2 installation from Flathub is located in the `usr/.var/app/io.github.ihhub.Fheroes2` directory.

For Windows users who use POEdit or a similar application, it is possible to compile the MO file using said program. However, note that
the program will need to be set to compile the MO file in the font encoding/charset that the language that you are translating to has been
set to.

For example, for German you will have to set font encoding to CP1252, while for Russian this would be CP1251. Later when submitting
a pull request with your changes, you will have to save the PO file in UTF-8 encoding because this is what Github supports.


### Sharing your translation work

When you are satisfied with your work, you can proceed with sharing it. The first step is to commit your work into the branch you made of 
your own fheroes2 fork. Then, create a pull request that proposes to introduce your changes into the fheroes2 repository.

How to [**commit**](https://github.com/git-guides/git-commit). 

How to [**create a pull request from your fork**](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/creating-a-pull-request-from-a-fork).

The pull request title has to be something human-understandable. This allows the team to quickly identify the purpose of 
your work, I.E. "Update the Slovak translation".

<details>

<summary>Pull request restrictions - Please read</summary>

The fheroes2 team has set a maximum of 400 total modified lines for any pull request for translations. For contributors wanting to
add translated lines to a new language this has a maximum of 30 total modified lines for that first pull request.

These limitations have been set because every pull request needs to be reviewed by our team, and so changing too many lines at once will only slow this
process down. In addition, GitHub becomes increasingly difficult to navigate once too many changes, comments and so on are present within the
same pull request page, further slowing down the process of reviewing it.

Furthermore, we have decided on a minimum amount of 15 changed strings for a translation pull request. For languages that have translations that are more
or less complete, less than this amount can be accepted.

Preferably a pull request should contain a small amount of changes, about 100 lines, all focused on translating a specific part of the game - for
example creature names or castle buildings.

</details>


## Updating PO templates and translatable strings in PO files

Currently all PO files are automatically updated with new strings after each commit that brings changes to the ingame text. If for whatever reason
you still need to update strings locally, this can be achieved by running the command below in `src/dist/fheroes2` to generate a new portable object
template (POT) file. Windows users will need to set up an environment that lets them run `make`, like Windows Subsystem for Linux (WSL) or
[**Cygwin**](https://www.cygwin.com/)/[**MSYS2**](https://www.msys2.org/).

```bash
make pot
```

Once the POT file has been created, go to the `files/lang` folder and run the command below to update translatable strings in the PO files.

```bash
make merge
```

If you are using programs mentioned above like POEdit, then they have options to merge new strings from a POT file.