#!/bin/bash

# Script to determine if source code in Pull Request has proper copyright headers.
# Exits with non-zero exit code if formatting is needed.
#
# This script assumes to be invoked at the project root directory.

set -e -o pipefail

SCRIPT_DIR=$(dirname "${BASH_SOURCE[0]}")
HEADERS_DIR="$SCRIPT_DIR/copyright_headers"

C_LIKE_FILES_TO_CHECK=$(git diff --name-only HEAD^ | (grep -E ".*\.(cpp|cc|c\+\+|cxx|c|h|hpp|java|rc)$" || true) \
                                                   | (grep -v "^src/thirdparty/.*/.*" || true))
SCRIPT_FILES_TO_CHECK=$(git diff --name-only HEAD^ | (grep -E ".*(\.(sh|py|ps1)|CMakeLists.txt|Makefile[^/]*|Android.mk|Application.mk)$" || true) \
                                                   | (grep -v "^src/thirdparty/.*/.*" || true))

if [ -z "$C_LIKE_FILES_TO_CHECK" ] && [ -z "$SCRIPT_FILES_TO_CHECK" ]; then
  echo "No source code to check if the copyright headers are correct."
  exit 0
fi

if [ -n "$C_LIKE_FILES_TO_CHECK" ]; then
  C_LIKE_FORMAT_DIFF=$(python3 "$SCRIPT_DIR/check_copyright_headers.py" "$HEADERS_DIR/full_header_c_like.txt" \
                                                                        "$HEADERS_DIR/header_template_c_like.txt" \
                                                                        $C_LIKE_FILES_TO_CHECK)
fi
if [ -n "$SCRIPT_FILES_TO_CHECK" ]; then
  SCRIPT_FORMAT_DIFF=$(python3 "$SCRIPT_DIR/check_copyright_headers.py" --handle-shebang \
                                                                        "$HEADERS_DIR/full_header_script.txt" \
                                                                        "$HEADERS_DIR/header_template_script.txt" \
                                                                        $SCRIPT_FILES_TO_CHECK)
fi

if [ -z "$C_LIKE_FORMAT_DIFF" ] && [ -z "$SCRIPT_FORMAT_DIFF" ]; then
  echo "All source code in PR has proper copyright headers."
  exit 0
else
  echo "Found invalid copyright headers!"
  [ -n "$C_LIKE_FORMAT_DIFF" ] && echo "$C_LIKE_FORMAT_DIFF"
  [ -n "$SCRIPT_FORMAT_DIFF" ] && echo "$SCRIPT_FORMAT_DIFF"
  exit 1
fi
