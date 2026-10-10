// The battery: the board reads its voltage on one pin (through a divider).
// A rough percent comes from the usual LiPo curve; "charging" is a guess from
// the voltage rising (the board has no charge-status wire).
#pragma once
#include <Arduino.h>

void batBegin();
void batPoll();                  // call often; samples every few seconds
bool batPresent();               // a battery is connected (not just the USB cable)
int batPercent();                // 0-100, smoothed; -1 = no reading yet
bool batCharging();              // plugged in and charging (a guess)
int batMilliVolts();             // the battery, in millivolts

// the percent for a resting LiPo cell at this voltage (also used by the picture-drawing program)
int batPercentFor(int mv);
