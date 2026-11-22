#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p linux/build
cc -O2 -Ivendor/silk/interface -Ivendor/silk/src -Dmain=silk_decoder_main -c vendor/silk/test/Decoder.c -o linux/build/decoder.o
cc -O2 -Ivendor/silk/interface -Ivendor/silk/src linux/main.c vendor/silk/src/*.c linux/build/decoder.o $(pkg-config --cflags --libs gtk+-3.0) -lm -o linux/build/silk-converter
