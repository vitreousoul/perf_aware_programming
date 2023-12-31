#!/usr/bin/env sh

DEBUG=1
# SOURCE_FILES="src/haversine_process.c"
SOURCE_FILES="src/repitition_testing.c"
SETTINGS="-std=c99 -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wno-comment"
OUTPUT_DIR="dist"

if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir $OUTPUT_DIR
fi

if [ $DEBUG -eq 0 ]; then
    echo "Optimized build";
    TARGET="-O2 -o $OUTPUT_DIR/haversine.exe"
elif [ $DEBUG -eq 1 ]; then
    echo "Debug build";
    TARGET="-g -O1 -o $OUTPUT_DIR/haversine.out"
fi

echo $TARGET
echo $SETTINGS
echo $SOURCE_FILES

gcc $TARGET $SETTINGS $SOURCE_FILES
