#!/bin/bash

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2021 - 2024                                             #
#                                                                         #
#   Copyright 2018 Google LLC                                             #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

# Script to determine whether the source code in the Pull Request is formatted correctly.
# Exits with a non-zero exit code if formatting is needed.
#
# It is assumed that this script is called from the project root directory.

set -e -o pipefail

FILES_TO_CHECK=$(git diff --name-only HEAD^ | (grep -E ".*\.(cpp|cc|c\+\+|cxx|c|h|hpp|java)$" || true) \
                                            | (grep -v "^src/thirdparty/.*/.*" || true))

if [ -z "$FILES_TO_CHECK" ]; then
  echo "There is no source code to check the formatting."
  exit 0
fi

if FORMAT_DIFF=$(git diff -U0 HEAD^ -- $FILES_TO_CHECK | clang-format-diff -p1 -style=file) && [ -z "$FORMAT_DIFF" ]; then
  echo "All the source code in the PR is formatted correctly."
  exit 0
else
  echo "Formatting errors found!"
  echo "$FORMAT_DIFF"
  exit 1
fi
