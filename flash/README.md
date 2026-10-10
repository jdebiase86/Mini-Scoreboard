# Putting a program on the mini from a web browser

No Arduino IDE needed. Works in Chrome or Edge on a Mac or a Windows PC
(not Safari, not on a phone).

Files here:
- mini-hw-test-full.bin: the hardware test (screen, touch, LED, speaker,
  battery reading, Wi-Fi). Walks through 8 tests on the screen.
- mini-0.3-full.bin: the Mini Scoreboard itself, stage 1 (screen, touch,
  Wi-Fi setup, home screen, updates over Wi-Fi). Once this is on, later
  versions install themselves over Wi-Fi.

Steps:
1. On this page on GitHub, tap the file you want, then the download button
   (arrow pointing down, top right of the file view).
2. Plug the mini into the computer with a USB-C cable that carries data
   (some charging cables don't).
3. In Chrome, open https://espressif.github.io/esptool-js/
4. Under Program, tap Connect. Pick the port that appears (it often says
   "USB Serial" or "CH340"; on a Mac it may say cu.usbserial). Tap Connect.
5. Flash Address stays 0x0. Choose File: pick the file from step 1.
6. Tap Program. It takes about a minute. When the log says "Leaving...",
   tap Disconnect, unplug the mini and plug it back in.

If no port shows up in step 4: try another USB-C cable, then another USB
port. If it says it can't connect: hold the board's BOOT button, tap Program
again, and let go of BOOT once the progress starts.

Putting a new program on this way wipes the mini's saved Wi-Fi, teams and
touch setup, so it starts fresh. Updates over Wi-Fi keep them.
