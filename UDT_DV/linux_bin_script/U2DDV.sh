#!/bin/bash
export LD_LIBRARY_PATH="./"
export LD_PRELOAD="libQtCore4.so libQtGui4.so"
exec ./U2DDV "$@"

