#include "7segmentdisplay.h"
#include <Arduino.h>

/********PIN VARIABLES************/
//volatile const int latchPin = 26;  // Latch pin of 74HC595 is connected to Digital pin 6
//volatile const int clockPin = 27; // Clock pin of 74HC595 is connected to Digital pin 7
//volatile const int dataPin = 25; 
/********PIN VARIABLES************/
volatile int currentdigit = 0;
#define latchPin 6
#define clockPin 7 
#define dataPin 5

//bytes for display
volatile byte leds = 0;    // Variable to hold the pattern of which LEDs are currently turned on or off
volatile byte digit = 0;
volatile uint16_t   secondsToshow = 0;
volatile bool       shouldTimerFlash = false;
uint32_t showTextSince = 0;
bool showText = false;
uint16_t showTextFor = 3000;
bool resetTimer = false;
char* textToshow;



void DisplayBegin(){
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);  
    pinMode(clockPin, OUTPUT);
}

void DisplayLoop()
{
    if(!showText)
    {
       ShowTime(secondsToshow / 60, secondsToshow % 60, shouldTimerFlash);
    }
    else if(showText && (millis()-showTextSince) < showTextFor)
    {   
       // Serial.print("showing:");
       //Serial.println(textToshow);
        ShowText(textToshow, false);
    }
    else if(showText && (millis()-showTextSince) > showTextFor)
    {   
        showTextSince = millis();
        showText = false;
    }else{
      ShowTime(secondsToshow / 60, secondsToshow % 60, shouldTimerFlash);
    }
    
    //ShowTime(11, 11, false);
    //Serial.println("1");
    //ShowDigit(0, 0, true);
}

void UpdateShiftRegister()
{
    digitalWrite(latchPin, LOW); 
    shiftOut(dataPin, clockPin, LSBFIRST, leds);  
    shiftOut(dataPin, clockPin, LSBFIRST, digit);  
    digitalWrite(latchPin, HIGH);
    //Serial.println("2");
}

void ShowText(char* text, uint16_t showFor, bool dot){
    textToshow = text;
    showTextFor = showFor;
    showText = true;
    showTextSince = millis();
}
void ShowTime(uint16_t seconds, bool isCounting){  
    secondsToshow = seconds;
    shouldTimerFlash = isCounting;
}

void ShowText(char* text, bool dot){
    if(currentdigit == 0)
    {
        ShowChar(textToshow[currentdigit], currentdigit, dot);
        currentdigit++;
    }
    else if(currentdigit == 1)
    {
        ShowChar(textToshow[currentdigit], currentdigit, dot);
        currentdigit++;
    }
    else if(currentdigit == 2)
    {
        ShowChar(textToshow[currentdigit], currentdigit, dot);
        currentdigit++;
    }
    else if(currentdigit == 3)
    {
        ShowChar(textToshow[currentdigit], currentdigit, dot);
        currentdigit++;
    }
    else{
        currentdigit = 0;
    }
}
void ShowChar(char Char, int digitIndex, bool dot){
    digit = 0;
    leds = 0;
    switch(digitIndex){
    case 0:   
        digit = B01110000;   
        break;
    case 1:   
        digit = B10110000;   
        break;
    case 2:   
        digit = B11010000;   
        break;
    case 3:   
        digit = B11100000;   
        break;
    }

    switch(Char){
    case 'a':   
        leds = B11101110;   
        break;
    case 'u':   
        leds = B01111100;   
        break;
    case 'o':   
        leds = B00111010;   
        break;
    case 'f':   
        leds = B10001110;
        break;
    case 'd':   
        leds = B01111010;  
        break; 
    case 'n':   
        leds = B00101010;  
        break;
    case 'e':   
        leds = B10011110;  
        break;  
    case 'r':   
        leds = B00001010;  
        break;
    case 's':   
        leds = B10110110;  
        break;
    case 't':   
        leds = B00011110;  
        break;    
    }

    if(dot){
        bitSet(leds, 0);
    }
    UpdateShiftRegister();
}

void ShowDigit(int numDigit, int digitIndex, bool dot){
  
    digit = 0;
    leds = 0;
    switch(digitIndex){
    case 0:   
        digit = B01110000;   
        break;
    case 1:   
        digit = B10110000;   
        break;
    case 2:   
        digit = B11010000;   
        break;
    case 3:   
        digit = B11100000;   
        break;
    }    
   
    byte NUMBERS[10] = {B11111100,B01100000,B11011010,B11110010,B01100110,B10110110,B10111110,B11100000,B11111110,B11110110};    
    leds = NUMBERS[numDigit];
    if(dot){
        bitSet(leds, 0);
    }

    UpdateShiftRegister();
 }
void ShowTime(int minute, int sec, bool flash){
    if(!(sec < 10))
    {
        ShowDigit(sec % 10,3,(sec % 2) == 1 ? true : false);
        ShowDigit(sec / 10,2,false);            
    }
    else
    {
        ShowDigit(sec,3,(sec % 2) == 1 ? true : false);
        ShowDigit(0,2,false);
    }
    if(!(minute < 10))
    {
        ShowDigit(minute % 10,1,true);
        ShowDigit(minute / 10,0,false);
    }
    else
    {
        ShowDigit(minute,1,true);
        ShowDigit(0,0,false);
    }

    if(currentdigit == 0){
        if(!(minute < 10))
        {     
            ShowDigit(minute / 10,0,false);
        }
        else
        {
            ShowDigit(0,0,false);
        }
        currentdigit++;
    }
    else if(currentdigit == 1){
        if(!(minute < 10))
        {
            ShowDigit(minute % 10,1,true);        
        }
        else
        {
            ShowDigit(minute,1,true);
        }
        currentdigit++;
    }
    else if(currentdigit == 2){
        if(!(sec < 10))
        {
          
            ShowDigit(sec / 10,2,false);            
        }
        else
        { 
            ShowDigit(0,2,false);
        }
        currentdigit++;
    }
    else if(currentdigit == 3){
        if(!(sec < 10))
        {
            ShowDigit(sec % 10,3,(sec % 2) == 1 ? true : false);
        }
        else
        {
            ShowDigit(sec,3,(sec % 2) == 1 ? true : false);         
        }
        currentdigit++;
    }
    else
    {
        currentdigit = 0; 
    }
   
}
