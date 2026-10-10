#!/bin/sh
# Rebuilds firmware/mini/mn_sudoku_bank.h (the ready-made Sudoku puzzles).
set -e
cd "$(dirname "$0")"
mkdir -p out
g++ -std=gnu++17 -O2 -w make_sudoku_bank.cpp -o out/make_sudoku_bank
out/make_sudoku_bank
