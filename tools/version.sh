#!/usr/bin/env bash

if [[ "$#" -eq 0 ]]; then
    echo "Usage: ./version.sh VERSION_FILE [INCLUDE_DEST]"
    echo ""
    echo "If INCLUDE_DEST is not given, then the contents of VERSION_FILE"
    echo "will be written to standard output."
    exit 1
fi

VERSION_FILE=$1
INCLUDE_DEST=$2

VERSION=$(cat "${VERSION_FILE}")

if [[ -n "$INCLUDE_DEST" ]]; then
    {
        echo "/* THIS FILE IS GENERATED; DO NOT MODIFY!!! */"
        echo
        echo "#ifndef METANG_VERSION_H"
        echo "#define METANG_VERSION_H"
        echo
        echo "#define METANG_VERSION \"${VERSION}\""
        echo
        echo "#endif /* METANG_VERSION_H */"
    } >"${INCLUDE_DEST}"
else
    echo "${VERSION}"
fi
