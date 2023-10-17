//
// Made by Kyrat - 2021-11-15
// Modified: 2023.01.16
//

#include <Arduino.h>
#include <OneButton.h>
#include <RotaryEncoder.h>
#include "7segmentdisplay.h"


#define PIN_IN1 2
#define PIN_IN2 3
#define buttonPin 4
#define sensorHighPin A0
#define sensorLowPin 9
#define outputPin 8
#define modeSelector 10
#define debug


uint16_t incraseTimerbyRotation = 10;
uint16_t maxTimeEllapsetoFill = 15;
uint16_t minTimeEllapsetoFill = 5;
uint16_t defaultFillTimer = 300;
uint32_t newFillTimer = defaultFillTimer;
//timer variables


bool manualMode = false;
bool prevmanualModeSwitchState = true;
bool levelSwitchStarted = false;
bool isLevelStartCountdownToStart = false;
bool isRunning = false;
bool hadError = false;
volatile uint32_t remainingRunTimer = defaultFillTimer;
volatile uint32_t remainingCountDownTimer = minTimeEllapsetoFill;




OneButton levelHighSensor(sensorHighPin, false);
OneButton levelLowSensor(sensorLowPin, false);
OneButton encoderButton(buttonPin);

int i = 0;
bool doneCount = false;

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

    encoderButton.attachLongPressStart([] { encoderPressed(false); });
    encoderButton.attachLongPressStart([] { encoderPressed(true); });
    encoderButton.attachClick([] { encoderClicked(); });



    levelLowSensor.setPressTicks(20); 
    levelHighSensor.setPressTicks(20); 
    levelLowSensor.attachLongPressStart([] { sensorStateChanged(false, false); });
    levelLowSensor.attachLongPressStop([] { sensorStateChanged(false, true); });
    levelHighSensor.attachLongPressStart([] { sensorStateChanged(true, false); });
    levelHighSensor.attachLongPressStop([] { sensorStateChanged(true, true); });
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
    
    DisplayBegin();
    pinMode(outputPin, OUTPUT);
    pinMode(modeSelector, INPUT);
    
    Serial.begin(9600);
    prevmanualModeSwitchState = !digitalRead(modeSelector);
    //sensorStateChanged(false, false);
}

ISR(TIMER1_COMPA_vect){    
   
        i++;
        if(i>=100){
            i = 0;
            if(isRunning){
                Serial.print("running ");
                if(levelSwitchStarted){                    
                    if(!isLevelStartCountdownToStart){
                        if(remainingRunTimer > 0){
                            Serial.print("auto:");
                            remainingRunTimer--;
                            Serial.print(remainingRunTimer);
                            Serial.print("\n");
                        }
                    }
                }
                else if(manualMode){
                    Serial.print("manual: ");
                    if(remainingRunTimer > 0){
                        remainingRunTimer--;
                        Serial.print(remainingRunTimer);
                        Serial.print("\n");
                    }
                }
                else{
                    ;
                }
            }
            else
            {
                Serial.print("countdown: ");
                if(isLevelStartCountdownToStart){
                    if(remainingCountDownTimer > 0){
                        Serial.print(remainingCountDownTimer);
                        Serial.print("\n");
                        remainingCountDownTimer--;
                    }
                }
            }
        }
}

void encoderPressed(bool released){
    ResetTimer();
}

void encoderClicked(){
    if(manualMode){
        if(levelSwitchStarted){
            levelSwitchStarted = false;
            isLevelStartCountdownToStart = false;
        }
        // a manual mode alap kiugrás?
        if(!isRunning && !hadError){
            isRunning = true;
            remainingRunTimer = newFillTimer;
        }
        else
        {
            isRunning = false;
        }
        
    }
    else
    {
        ShowText("auto", 1000, false);
    }
  
}

//true - kell töltés
//false - tele
void sensorStateChanged(bool highSide, bool signalLevel){

    bool highSensor = digitalRead(sensorHighPin);
    bool lowhSensor = digitalRead(sensorLowPin);
    Serial.println("statechange");
    Serial.print("High: ");
    Serial.println(highSensor);
    Serial.print("Low: ");
    Serial.println(lowhSensor);
    if(!manualMode)
    {     Serial.println("in automode");
        // Ha lent van a vízszint, és nem megy a leszámolás
        if( !lowhSensor &&
            !highSensor && 
            !isLevelStartCountdownToStart && 
            !hadError &&
            !isRunning)
        {
            Serial.println("Full fillup.");
            levelSwitchStarted = true;
            isLevelStartCountdownToStart = true;
            remainingCountDownTimer = minTimeEllapsetoFill;
        }
        // Ha a vízszint elérte az alsót x időn belül, leáll a számolás.
        else if(
            lowhSensor &&
            !highSensor &&
            isLevelStartCountdownToStart)
        {
            Serial.println("Low sensor bouncing in time.");
            isRunning = false;
            levelSwitchStarted = false;
            isLevelStartCountdownToStart = false;
            remainingCountDownTimer = minTimeEllapsetoFill;
        }
        // Ha a vízszint elérte az elsőt, de már a töltés közben.
        else if(
            lowhSensor &&
            !highSensor &&
            !isLevelStartCountdownToStart && 
            levelSwitchStarted &&
            isRunning)
        {
           Serial.println("Water at low sens, during the filling.");
           ;
        }
        else if(
            !lowhSensor &&
            !highSensor &&
            !isLevelStartCountdownToStart && 
            isRunning)
        {
           Serial.println("Low sensor bouncing, during the filling.");
           ;
        }
        // Ha elérte mind2 szintet, és megy, leáll.
        else if(
            highSensor &&
            !isLevelStartCountdownToStart && 
            isRunning)
        {
            Serial.println("Both reached max.");
            isRunning = false;
            levelSwitchStarted = false;
            isLevelStartCountdownToStart = false;
            remainingCountDownTimer = minTimeEllapsetoFill;
            remainingRunTimer = maxTimeEllapsetoFill;
        }
        else
        {
            Serial.println("other option.");
            if(highSensor)
            {
                isRunning = false;
                levelSwitchStarted = false;
                isLevelStartCountdownToStart = false;
                remainingCountDownTimer = minTimeEllapsetoFill;  
                remainingRunTimer = maxTimeEllapsetoFill;
            }
        }
        if (hadError)
        {
                isRunning = false;
                levelSwitchStarted = false;
                isLevelStartCountdownToStart = false;
                remainingCountDownTimer = minTimeEllapsetoFill;  
                remainingRunTimer = maxTimeEllapsetoFill;
        }
       
    }
    
}

void loop() 
{
  bool mode = digitalRead(modeSelector);
  if(mode != prevmanualModeSwitchState)
  {
      prevmanualModeSwitchState = mode;
      manualMode = mode;
      isRunning = false;
    
     
      if(manualMode)
      {
         remainingRunTimer = defaultFillTimer; 
      }
      else
      {
        levelSwitchStarted = false;
        isLevelStartCountdownToStart = false;
        remainingCountDownTimer = minTimeEllapsetoFill;
        remainingRunTimer = maxTimeEllapsetoFill; 
        sensorStateChanged(false, false);
      }
  }
    levelHighSensor.tick();
    levelLowSensor.tick();
    encoderButton.tick();
  
    Encoder();
    DisplayLoop();

    if(isRunning){
        digitalWrite(outputPin,HIGH);
    }
    else{
        digitalWrite(outputPin,LOW);
    }


    if(!hadError){
        if( levelSwitchStarted && 
            isLevelStartCountdownToStart && 
            remainingCountDownTimer <= 0 )
        {
            isLevelStartCountdownToStart = false;
            isRunning = true;
            remainingCountDownTimer = minTimeEllapsetoFill;
            remainingRunTimer = maxTimeEllapsetoFill;
        }
        else if(levelSwitchStarted && remainingRunTimer <= 0)
        {
            levelSwitchStarted = false;
            isLevelStartCountdownToStart = false;
            isRunning = false;   
            hadError = true;
            ShowText("err ", 360000000, false);
        }
        else if(remainingRunTimer <= 0)
        {
            isRunning = false; 
            ShowText("done", 1000, false);
            remainingRunTimer = newFillTimer;
        }
        if(levelSwitchStarted && !manualMode)
        {
            if(isLevelStartCountdownToStart)
            {
                ShowTime(remainingCountDownTimer, isRunning);
            }
            else
            {
                ShowTime(remainingRunTimer, isRunning);
            }
        }
        else
        {
            ShowTime(remainingRunTimer, isRunning);
        }
    }
    else
    {
        isRunning = false;
        ShowText("err ", 1000, false);
    }
}

void ResetTimer(){
    ShowText("rst ", 1000, false);
}

void SPIread(){

}

void SPIsave(){

}

void Encoder(){
    static int pos = 0;
    int newPos = encoder->getPosition();
    if (pos != newPos) 
    {
        int dir = (int)(encoder->getDirection());
        if(!isRunning && manualMode){
          
            if((newPos%2) == 0)
            {
                if (dir==1) 
                {
                    if(remainingRunTimer > incraseTimerbyRotation){
                        if ((remainingRunTimer)%incraseTimerbyRotation!=0)
                        {
                            remainingRunTimer -= (remainingRunTimer)%incraseTimerbyRotation;                        
                        }else{
                            remainingRunTimer-=incraseTimerbyRotation;
                        }
                    }
                }                
                if (dir==-1) {  
                    if((remainingRunTimer)<=(3600-incraseTimerbyRotation)){
                        if (remainingRunTimer%incraseTimerbyRotation!=0){
                            remainingRunTimer += incraseTimerbyRotation - (remainingRunTimer%incraseTimerbyRotation);
                        } 
                        else{
                            remainingRunTimer+=incraseTimerbyRotation;
                        }
                    }
                }
                newFillTimer = remainingRunTimer;
            }
        }
        else if(!manualMode)
        {
            ShowText("auto", 1000, false);
        }
        else if(isRunning)
        {
            ShowText("run ", 1000, false);
        }
        pos = newPos;    
    }
}
