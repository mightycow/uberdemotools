#!/bin/bash
cd ../build/gmake
make clean
make UDT config=release_x32
make UDT config=release_x64
