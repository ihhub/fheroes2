#!/bin/bash

# Uninstall SDL 2
brew remove sdl2_image
brew remove sdl2_mixer
brew remove sdl2_ttf
brew remove sdl2

# Install SDL 1.2
brew install sdl
brew install sdl_ttf
brew install sdl_mixer
brew install sdl_image
