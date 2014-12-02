#!/bin/bash
cd ../.build/gmake
make clean
make UDT config=release_x64
make UDT_cutter config=release_x64
make UDT_splitter config=release_x64
make UDT_test_addons config=release_x64
