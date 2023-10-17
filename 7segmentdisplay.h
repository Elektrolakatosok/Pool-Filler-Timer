#ifndef SEVENSEGMENTDISPLAY_H
#define SEVENSEGMENTDISPLAY_H
#include <Arduino.h>
void UpdateShiftRegister();
void ShowChar(char Char, int digitIndex, bool dot);
void ShowDigit(int numDigit, int digitIndex, bool dot);
void ShowTime(int minute, int sec, bool flash);
void DisplayLoop();
void DisplayBegin();
void ShowText(char* text, uint16_t showFor, bool dot);
void ShowTime(uint16_t seconds, bool isCounting);
void ShowText(char* text, bool dot);
#endif
