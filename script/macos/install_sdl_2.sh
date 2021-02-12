#!/bin/bash

# Uninstall dev tools
brew remove gettext

# Install dev tools
brew install gettext

# Uninstall SDL 1.2
brew remove sdl_image
brew remove sdl_mixer
brew remove sdl_ttf
brew remove sdl

# Install SDL 2
brew install sdl2
brew install sdl2_ttf
brew install sdl2_mixer
brew install sdl2_image
