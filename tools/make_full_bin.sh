#!/bin/sh
# Joins a build's bootloader, partition table and program into one file that
# goes on a blank (or any) board at address 0 - the file for the browser
# flasher (flash/README.md).
#   tools/make_full_bin.sh <build dir> <sketch name> <out.bin>
set -e
B=$1; N=$2; OUT=$3
CORE=$(ls -d "$HOME"/.arduino15/packages/esp32/hardware/esp32/2.0.9)
ESPTOOL=$(ls -d "$HOME"/.arduino15/packages/esp32/tools/esptool_py/*/esptool.py | head -1)
python3 "$ESPTOOL" --chip esp32 merge_bin -o "$OUT" \
  --flash_mode dio --flash_freq 80m --flash_size 4MB \
  0x1000 "$B/$N.ino.bootloader.bin" \
  0x8000 "$B/$N.ino.partitions.bin" \
  0xe000 "$CORE/tools/partitions/boot_app0.bin" \
  0x10000 "$B/$N.ino.bin"
ls -la "$OUT"
