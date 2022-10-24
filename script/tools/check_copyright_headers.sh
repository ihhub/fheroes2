#!/bin/bash

# Script to determine if source code in Pull Request has proper copyright headers.
# Exits with non-zero exit code if formatting is needed.
#
# This script assumes to be invoked at the project root directory.

set -e -o pipefail

SCRIPT_DIR=$(dirname "${BASH_SOURCE[0]}")
HEADERS_DIR="$SCRIPT_DIR/copyright_headers"

SRC_FILES_TO_CHECK=$(git diff --name-only HEAD^ | (grep -E ".*\.(cpp|cc|c\+\+|cxx|c|h|hpp|java|rc)$" || true) \
                                                | (grep -v "^src/thirdparty/.*/.*" || true))
MAKEFILES_TO_CHECK=$(git diff --name-only HEAD^ | (grep -E ".*(CMakeLists.txt|Makefile[^/]*|Android.mk|Application.mk)$" || true) \
                                                | (grep -v "^src/thirdparty/.*/.*" || true))

if [ -z "$SRC_FILES_TO_CHECK" ] && [ -z "$MAKEFILES_TO_CHECK" ]; then
  echo "No source code to check if the copyright headers are correct."
  exit 0
fi

if [ -n "$SRC_FILES_TO_CHECK" ]; then
  SRC_FORMAT_DIFF=$(python3 "$SCRIPT_DIR/check_copyright_headers.py" "$HEADERS_DIR/full_header_src.txt" \
                                                                     "$HEADERS_DIR/header_template_src.txt" \
                                                                     $SRC_FILES_TO_CHECK)
fi
if [ -n "$MAKEFILES_TO_CHECK" ]; then
  MKF_FORMAT_DIFF=$(python3 "$SCRIPT_DIR/check_copyright_headers.py" "$HEADERS_DIR/full_header_mkf.txt" \
                                                                     "$HEADERS_DIR/header_template_mkf.txt" \
                                                                     $MAKEFILES_TO_CHECK)
fi

if [ -z "$SRC_FORMAT_DIFF" ] && [ -z "$MKF_FORMAT_DIFF" ]; then
  echo "All source code in PR has proper copyright headers."
  exit 0
else
  echo "Found invalid copyright headers!"
  [ -n "$SRC_FORMAT_DIFF" ] && echo "$SRC_FORMAT_DIFF"
  [ -n "$MKF_FORMAT_DIFF" ] && echo "$MKF_FORMAT_DIFF"
  exit 1
fi
