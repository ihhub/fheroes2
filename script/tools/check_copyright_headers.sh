#!/bin/bash

# Script to determine if source code in Pull Request has proper copyright headers.
# Exits with non-zero exit code if formatting is needed.
#
# This script assumes to be invoked at the project root directory.

FILES_TO_CHECK=$(git diff --name-only HEAD^ | grep -E ".*\.(cpp|cc|c\+\+|cxx|c|h|hpp)$")

if [ -z "$FILES_TO_CHECK" ]; then
  echo "No source code to check if the copyright headers are correct."
  exit 0
fi

FORMAT_DIFF=$(python3 "$(dirname "${BASH_SOURCE[0]}")/check_copyright_headers.py" $FILES_TO_CHECK)

if [ -z "$FORMAT_DIFF" ]; then
  echo "All source code in PR has proper copyright headers."
  exit 0
else
  echo "Found invalid copyright headers!"
  echo "$FORMAT_DIFF"
  exit 1
fi
