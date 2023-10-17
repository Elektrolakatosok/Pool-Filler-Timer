//
// Made by Kyrat - 2021-11-15
//

#include <Arduino.h>
#include <RotaryEncoder.h>

//pin definitions
int latchPin = 6;  // Latch pin of 74HC595 is connected to Digital pin 6
int clockPin = 7; // Clock pin of 74HC595 is connected to Digital pin 7
int dataPin = 5;  // Data pin of 74HC595 is connected to Digital pin 5
#define PIN_IN1 2
#define PIN_IN2 3
#define LEVEL_HIGH 10
#define LEVEL_LOW 11
int buttonPin = 4;
int outputPin = 8;

//bytes
byte leds = 0;    // Variable to hold the pattern of which LEDs are currently turned on or off
byte digit = 0;

//timer variables
int secEllapsed = 0;
int i = 0;
int countToSec = 0;
int OrigcountToSec = 100;
bool isRunning = false;
bool doneCount = false;
int incraseSec = 10;
int hardcodeddefault = 100;

//button manage
unsigned long lastpressed = 0;
unsigned long lastfelengedett = 0;
bool prevPressedState = false;
bool isHolding = false;
bool pressedandrelease = false;

//show variables
long showTextSince = 0;
bool showText = false;
int showTextFor = 3000;
bool resetTimer = false;


//Water levels
bool levelHigh = false;
bool levelLow = false;
bool autoStarted = false;
bool overFlowStopped = false;
RotaryEncoder *encoder = nullptr;



void checkPosition()
{  
    encoder->tick(); // just call tick() to check the state.   
}


void setup()
{  
 
    encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);
    attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);
    
    countToSec = OrigcountToSec;
    // TIMER 1 for interrupt frequency 100 Hz:
    cli(); // stop interrupts
    TCCR1A = 0; // set entire TCCR1A register to 0
    TCCR1B = 0; // same for TCCR1B
    TCNT1  = 0; // initialize counter value to 0
    // set compare match register for 100 Hz increments
    OCR1A = 19999; // = 16000000 / (8 * 100) - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS12, CS11 and CS10 bits for 8 prescaler
    TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);
    sei(); // allow interrupts
    
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);  
    pinMode(clockPin, OUTPUT);
    pinMode(outputPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(LEVEL_HIGH, INPUT_PULLUP);
  pinMode(LEVEL_LOW, INPUT_PULLUP);
    Serial.begin(9600);

}

ISR(TIMER1_COMPA_vect){    
    if(isRunning){
        i++;
        if(i>=100){
            secEllapsed++;
            i = 0;        
            Serial.println(secEllapsed);
        }
        if(secEllapsed>=32000){
            secEllapsed = 0;
        }
    }
}

void loop() 
{
  LevelSwitchCheck();
    static int pos = 0;
    int newPos = encoder->getPosition();
    if (pos != newPos) {
        int dir = (int)(encoder->getDirection());
        /* 
        Serial.print("pos:");
        Serial.print(newPos%2);
        Serial.print("pos:");
        Serial.print(newPos);
        Serial.print(" dir:");
        Serial.println(dir);*/
        if(!isRunning){
          
            if((newPos%2) == 0){

            if (dir==1) {

                if((countToSec-secEllapsed) > incraseSec){
                    if ((countToSec-secEllapsed)%incraseSec!=0)
                    {
                        countToSec -= (countToSec-secEllapsed)%incraseSec;                        
                    }else{
                        countToSec-=incraseSec;
                    }
                }
                
            }                
            if (dir==-1) {  
                if((countToSec-secEllapsed)<=(3600-incraseSec)){
                   
                   
                    if ((countToSec-secEllapsed)%incraseSec!=0){
                       countToSec += incraseSec - ((countToSec-secEllapsed)%incraseSec);
                       /* code */
                    } 
                   else{
                        countToSec+=incraseSec;
                    }
                }
            }
        }
          }
      
        showText = false;
        pos = newPos;    
    }

    if(digitalRead(buttonPin)==LOW)
    {
        if(!isHolding){
            lastpressed = millis();           
        }

        isHolding = true;
        showText = false;
        //Serial.print("pressed for:");
    }

    if(digitalRead(buttonPin)==HIGH && isHolding){                     
        lastfelengedett = millis();
        isHolding = false;           
        pressedandrelease = true;         
        /*
        Serial.print("pressed for:");
        Serial.println(lastfelengedett-lastpressed);*/
    }
  
    if(pressedandrelease){
        if(!isRunning){
            if((lastfelengedett-lastpressed<=1500)){
                isRunning = true;
                pressedandrelease = false;
                if (!doneCount)
                {
                  //lehet -ba glithcel, ki kell probaálni.
                  if((countToSec-secEllapsed)%incraseSec==0){
                      OrigcountToSec = (countToSec-secEllapsed);
                   }
                
                }
                
            }
             if((lastfelengedett-lastpressed>1500)){
                ResetTimer();
               
                pressedandrelease = false;
            }
        }
        else{
            isRunning = false;
            pressedandrelease = false;
            showText = true;
            doneCount = false;
            digitalWrite(outputPin,LOW);
        }
    }

    if(isRunning){
        digitalWrite(outputPin,HIGH);
    }

    if((countToSec-secEllapsed)<=0)
    {   
    if(autoStarted){
      overFlowStopped = true;
    }

        isRunning = false;
        digitalWrite(outputPin,LOW);
        showText =true;
        secEllapsed = 0;
        countToSec = OrigcountToSec;
        showTextSince = millis();
        doneCount = true;
       
    }

    if(!showText){
        ShowTime((countToSec-secEllapsed) / 60, (countToSec-secEllapsed) % 60, false);
    }
    else{
        if(doneCount){
            // 3mp-ig mutattja a done-t ha végzett
            if((showTextSince + showTextFor)>millis()){
                ShowChar('d',0,false); 
                ShowChar('o',1,false); 
                ShowChar('n',2,false);   
                ShowChar('e',3,false); 
            }            
            else{
                // 3mp letelt.
                doneCount  = false;
                showText = false;
            }        
        }
        else if(resetTimer){
            if((showTextSince + showTextFor)>millis()){
                ShowChar('r',1,false); 
                ShowChar('s',2,false); 
                ShowChar('t',3,false);   
            }  
            else{
                // 3mp letelt.
                resetTimer  = false;
                showText = false;
            }       
           
        }
        else{
            //ha showtext igaz, de nem végzett a leszámolással.(manual off)
            ShowChar('o',1,false); 
            ShowChar('f',2,false); 
            ShowChar('f',3,false);      
        }
    }
}


void ResetTimer(){
    showTextSince = millis();
    isRunning = false;
    secEllapsed = 0;
    countToSec = hardcodeddefault;
    showText = true;
    resetTimer = true;
}

void LevelSwitchCheck()
{
  if(digitalRead(LEVEL_LOW)==LOW){
    levelHigh = true;
  }
  else{
    levelHigh = false;
  }
  
  if(digitalRead(LEVEL_HIGH)==LOW){
    levelLow = true;
  }
  else{
    levelLow = false;
  }
  if(!overFlowStopped && (levelLow && ! levelHigh)){
    //start 
    isRunning = true;
    autoStarted = true;
    secEllapsed = 0;
    countToSec = 3600; // 1 hour if overflows;
    digitalWrite(outputPin,HIGH);
  }
  
  if((!levelLow && levelHigh)){
    if(autoStarted){
      autoStarted = false;
      isRunning = false;
      digitalWrite(outputPin,LOW);
      showText = true;
      secEllapsed = 0;
      countToSec = OrigcountToSec;
      showTextSince = millis();
      doneCount = true;
    }
  }
}

void ShowTime(int minute, int sec, bool flash){
    if(!(sec < 10))
    {
        ShowDigit(sec % 10,3,false);
        ShowDigit(sec / 10,2,false);            
    }
    else
    {
        ShowDigit(sec,3,false);
        ShowDigit(0,2,false);
    }
    if(!(minute < 10))
    {
        ShowDigit(minute % 10,1,(sec % 2) == 1 ? false : true);
        ShowDigit(minute / 10,0,false);
    }
    else
    {
        ShowDigit(minute,1,(sec % 2) == 1 ? false :true );
        ShowDigit(0,0,false);
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

void UpdateShiftRegister()
{
   digitalWrite(latchPin, LOW); 
   shiftOut(dataPin, clockPin, LSBFIRST, leds);  
   shiftOut(dataPin, clockPin, LSBFIRST, digit);  
   digitalWrite(latchPin, HIGH);
}
