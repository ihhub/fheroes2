#!/bin/bash

# Script to determine if source code in Pull Request has proper copyright headers.
# Exits with non-zero exit code if formatting is needed.
#
# This script assumes to be invoked at the project root directory.

set -e -o pipefail

SCRIPT_DIR=$(dirname "${BASH_SOURCE[0]}")
HEADERS_DIR="$SCRIPT_DIR/copyright_headers"

CXX_FILES_TO_CHECK=$(git diff --name-only HEAD^ | (grep -E ".*\.(cpp|cc|c\+\+|cxx|c|h|hpp|rc)$" || true) \
                                                | (grep -v "^src/thirdparty/.*/.*" || true))
MAKEFILES_TO_CHECK=$(git diff --name-only HEAD^ | (grep -E ".*(CMakeLists.txt|Makefile[^/]*|Android.mk|Application.mk)$" || true) \
                                                | (grep -v "^src/thirdparty/.*/.*" || true))

if [ -z "$CXX_FILES_TO_CHECK" ] && [ -z "$MAKEFILES_TO_CHECK" ]; then
  echo "No source code to check if the copyright headers are correct."
  exit 0
fi

if [ -n "$CXX_FILES_TO_CHECK" ]; then
  CXX_FORMAT_DIFF=$(python3 "$SCRIPT_DIR/check_copyright_headers.py" "$HEADERS_DIR/full_header_cxx.txt" \
                                                                     "$HEADERS_DIR/header_template_cxx.txt" \
                                                                     $CXX_FILES_TO_CHECK)
fi
if [ -n "$MAKEFILES_TO_CHECK" ]; then
  MKF_FORMAT_DIFF=$(python3 "$SCRIPT_DIR/check_copyright_headers.py" "$HEADERS_DIR/full_header_mkf.txt" \
                                                                     "$HEADERS_DIR/header_template_mkf.txt" \
                                                                     $MAKEFILES_TO_CHECK)
fi

if [ -z "$CXX_FORMAT_DIFF" ] && [ -z "$MKF_FORMAT_DIFF" ]; then
  echo "All source code in PR has proper copyright headers."
  exit 0
else
  echo "Found invalid copyright headers!"
  [ -n "$CXX_FORMAT_DIFF" ] && echo "$CXX_FORMAT_DIFF"
  [ -n "$MKF_FORMAT_DIFF" ] && echo "$MKF_FORMAT_DIFF"
  exit 1
fi
