#!/usr/bin/env bash

PROJECT_NAME="crux"

CC=""
if command -v ccache >/dev/null 2>&1; then
    CC="ccache "
fi
CC="$CC gcc"

CFLAGS="-std=gnu99 -O3 -g -Wall -Wextra"

DIR="$(mktemp -t $PROJECT_NAME.XXXX -d)"
GCC="$($CC -dumpversion)"

compile() {
    echo "Compiling $1..."
    out=$(echo "$1" | sed 's|^src/||; s|\.c$|.o|')
    mkdir -p $(dirname "$DIR/$out")
    $CC -c $1 -Iinclude -o $DIR/$out $CFLAGS
    OBJECTS="$OBJECTS $DIR/$out"
}

for src in $(find src -name "*.c"); do
    compile $src;
done

echo "Objects ($OBJECTS )"
echo "Linking..."
$CC -o $PROJECT_NAME.out $CFLAGS $OBJECTS -lgcc

if [[ $? -ne 0 ]]; then
    exit 1
fi

echo "Done! cleanup..."
rm -r $DIR
