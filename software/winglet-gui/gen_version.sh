#!/bin/bash

VERSION_HDR_FILE="$1"

if [ -z "$VERSION_HDR_FILE" ]; then
    echo "Usage: $0 [version header file]"
    exit 1
fi

if [ "$1" = "--save" ]; then
    if [ -z "$2" ]; then
        echo "Usage: $0 --save [path to .version_info]"
        exit 1
    fi
    VERSION_SAVE_FILE="$2"
    unset VERSION_HDR_FILE
fi

# Get the current git version
REPO_DIR="$(dirname "$(realpath "$0")")"
VERSION_OVERRIDE_FILE="$REPO_DIR/.version_info"

if [ -e "$VERSION_OVERRIDE_FILE" ]; then
    # Version file is provided, use that rather than trying git
    GIT_VERSION="$(cat "$VERSION_OVERRIDE_FILE")"

elif [ -n "$(git -C "$REPO_DIR" rev-parse --show-cdup 2> /dev/null)" ]; then
    # If this script is not in the root of the git repo, then this isnt our repo
    # Report unknown version
    GIT_VERSION="UNKNOWN"

# This is the root of the git repo. Try to get description
elif ! GIT_VERSION="$(git -C "$REPO_DIR" describe --always --dirty 2>/dev/null)"; then
    # If we couldn't get a description (not a git repo) fall back to unknown
    GIT_VERSION="UNKNOWN"
fi

# If we're told to save the version, just do that and exit early
if [ -n "$VERSION_SAVE_FILE" ]; then
    echo "$GIT_VERSION" > "$VERSION_SAVE_FILE"
    exit 0
fi

# Only update file if it does not match
if [ -e "$VERSION_HDR_FILE" ]; then
    # Grab first line of file (contains comment with the version used by the file)
    PREV_VERSION="$(head -n 1 "$VERSION_HDR_FILE")"

    # Strip leading comment
    PREV_VERSION="${PREV_VERSION#// }"
else
    # File doesn't exist, set to empty string
    PREV_VERSION=""
fi

if [ "$PREV_VERSION" = "$GIT_VERSION" ]; then
    # Don't need to update version file. Can exit now
    exit 0
fi

echo "Creating $(basename $1) with version '$GIT_VERSION'"

cat << EOF > "$VERSION_HDR_FILE"
// $GIT_VERSION
#ifndef __BUILDGEN__VERSION_AUTOGEN_H_
#define __BUILDGEN__VERSION_AUTOGEN_H_

#define GIT_VERSION "$GIT_VERSION"

#endif
EOF

