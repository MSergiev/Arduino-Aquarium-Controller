#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#include <Wire.h>
#include <RtcDS1307.h>

// -=-=-=- Adjustable Parameters -=-=-=-

// Light Intensity (0-255)
int coldMax = 245;    //Cold Light
int warmMax = 90;    //Warm Light
int moonMax = 3;      //Moonlight

//Twilight (Minutes/2)
int transitionLength = 15;    //Sunrise/Sundown Transition Duration
int totalTransition = transitionLength*2;

//Moonlight (Minutes)
int moonlightLength = 30;    //Moonlight Duration

//Times (Hours/Minutes)
//Sunrise
unsigned long sunriseHour      = 14;    //Sunrise Hour
unsigned long sunriseMinutes   = 00;    //Sunrise Minutes
//Sundown
unsigned long sundownHour      = 21;    //Sundown Hour
unsigned long sundownMinutes   = 30;    //Sundown Minutes

//Pumps
//Pump-1 Start
unsigned long pump1StartHour    = 07;    //Pump-1 Start Hour
unsigned long pump1StartMinutes = 00;    //Pump-1 Start Minutes
//Pump-1 Stop
unsigned long pump1StopHour     = 21;    //Pump-1 Stop Hour
unsigned long pump1StopMinutes  = 15;    //Pump-1 Stop Minutes

//Pump-2 Start
unsigned long pump2StartHour    = 21;    //Pump-2 Start Hour
unsigned long pump2StartMinutes = 25;    //Pump-2 Start Minutes
//Pump-2 Stop             
unsigned long pump2StopHour     = 07;    //Pump-2 Stop Hour
unsigned long pump2StopMinutes  = 15;    //Pump-2 Stop Minutes

//Button Action
boolean buttonState = LOW;
boolean buttonFlag;
boolean lastButtonState;
boolean buttonStateChange;
long buttonTime, resetTime;
boolean isDay;
boolean transitionFlag = 1;
long buttonActionMin = 120;    //Button Action When Pressed (Minutes)
long buttonActionMillis = buttonActionMin*60000;

// -=-=-=- End of Adjustable Parameters -=-=-=-

//Pins
const int buttonPin = 2;    //Button Lights State
const int ledPin = 4;       //Button Action Led (Blue)
const int coldPin = 5;      //Cold Light --> Leg 11-Q2-Red-Track 3
//const int cold1Pin = 6;   //Cold Light --> Leg 12-Q1-FREE !!!
const int pump1Pin = 7;     //Pump-1 ------> Leg 13-Q5-Relay-1(In1)
const int pump2Pin = 8;     //Pump-2 ------> Leg 14-Q6-Relay-2(In2)
const int moonPin = 9;      //Moonlight ---> Leg 15-Q3-Green-Track 4
const int warmPin = 10;     //Warm Light --> Lrg 16-Q4-Black-Track 5

//Other Variables
unsigned long sunriseTime = sunriseHour*3600+sunriseMinutes*60;
unsigned long sundownTime = sundownHour*3600+sundownMinutes*60;
unsigned long sunriseEnd = sunriseTime+transitionLength*2*60;
unsigned long sundownEnd = sundownTime+transitionLength*2*60;
unsigned long moonlightEnd = sundownEnd+moonlightLength*60;

unsigned long pump1StartTime = pump1StartHour*3600+pump1StartMinutes*60;
unsigned long pump1StopTime = pump1StopHour*3600+pump1StopMinutes*60;
unsigned long pump2StartTime = pump2StartHour*3600+pump2StartMinutes*60;
unsigned long pump2StopTime = pump2StopHour*3600+pump2StopMinutes*60;

unsigned int coldVal, moonVal, warmVal, pump1Val, pump2Val;
unsigned long currentTime;

//unsigned long testHour = 24;
//unsigned long testMinutes = 0;

int wait = 1000;
unsigned long delayTime;

RtcDS1307 Rtc;
RtcDateTime now;

//===========================SETUP=========================

void setup() 
{
    Serial.begin(9600);
    Rtc.Begin();
    Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low); 

    initialize();

    pinMode(coldPin, OUTPUT);
    pinMode(moonPin, OUTPUT);
    pinMode(warmPin, OUTPUT);
    pinMode(pump1Pin, OUTPUT);
    pinMode(pump2Pin, OUTPUT);
    pinMode(ledPin, OUTPUT);    
    pinMode(buttonPin, INPUT);

    powerUptransition();
//    transitionUp();
//    transitionDown();
transitionLength = transitionLength*60;
    
    Serial.println("CONTROLLER READY");
}

//========================LOOP=========================

void loop () 
{
    getSerialCommand();
    buttonCheck();
    if(millis() - delayTime >= wait || buttonStateChange) {
    delayTime = millis();
      if(Rtc.GetIsRunning()){
        now = Rtc.GetDateTime();
        currentTime = (unsigned long)now.Hour()*3600+(unsigned long)now.Minute()*60+(unsigned long)now.Second();       
        if(currentTime >= sunriseTime && currentTime <= sundownEnd){
          isDay = true;
        } else {
          isDay = false;
        }
        //currentTime = testHour*3600+testMinutes*60; 
        if(!buttonFlag){
          light();
        }  
        buttonFunction();  
        pumps();        
        analogWrite(coldPin, coldVal);
        analogWrite(moonPin, moonVal);
        analogWrite(warmPin, warmVal);
        digitalWrite(pump1Pin, pump1Val); 
        digitalWrite(pump2Pin, pump2Val); 
      } else {
        Serial.println("RTC Connection Lost!");
        digitalWrite(coldPin, !digitalRead(coldPin));
        digitalWrite(moonPin, !digitalRead(moonPin));
        digitalWrite(warmPin, !digitalRead(warmPin));
        digitalWrite(pump1Pin, 1);
        digitalWrite(pump2Pin, 1);
      }        
    }
}

void light(){
  if(currentTime > sunriseTime && currentTime <= sunriseEnd){
    sunrise();
  } else if(currentTime > sunriseEnd && currentTime <= sundownTime){
    warmVal = warmMax;
    coldVal = coldMax;
    moonVal = coldMax;
  } else if(currentTime > sundownTime && currentTime <= sundownEnd){
    sundown();
  } else if(currentTime > sundownEnd && currentTime <= moonlightEnd){
    warmVal = 0;
    coldVal = 0;     
    moonVal = moonMax;
  } else {
    warmVal = 0;
    coldVal = 0;
    moonVal = 0;
  }
}

void sunrise(){
     if(currentTime < sunriseTime+transitionLength){
        warmVal = map(currentTime,sunriseTime,sunriseTime+transitionLength,0,warmMax);
        coldVal = 0;
        moonVal = 0;
        Serial.print("- Sunrise Warm Value: ");
        Serial.print(warmVal);
        Serial.print("/");
        Serial.println(warmMax);
     } else {
        coldVal = map(currentTime,sunriseTime+transitionLength,sunriseTime+transitionLength*2,0,coldMax);
        moonVal = map(currentTime,sunriseTime+transitionLength,sunriseTime+transitionLength*2,0,coldMax);        
        warmVal = warmMax;
        Serial.print("- Sunrise Cold Value: ");
        Serial.print(coldVal);
        Serial.print("/");
        Serial.println(coldMax);
     }
}

void sundown(){
     if(currentTime < sundownTime+transitionLength){
        warmVal = map(currentTime,sundownTime,sundownTime+transitionLength,warmMax,0);
        coldVal = coldMax;
        Serial.print("- Sundown Warm Value: ");
        Serial.print(warmVal);
        Serial.print("/");
        Serial.println(warmMax);
     } else {
        coldVal = map(currentTime,sundownTime+transitionLength,sundownTime+transitionLength*2,coldMax,0);
        moonVal = map(currentTime,sundownTime+transitionLength,sundownTime+transitionLength*2,coldMax,moonMax);        
        warmVal = 0;
        Serial.print("- Sundown Cold Value: ");
        Serial.print(coldVal);
        Serial.print("/");
        Serial.println(coldMax);
     }
}

void pumps(){
  if((currentTime >= pump1StartTime && currentTime < pump1StopTime) || (pump1StartTime > pump1StopTime && (currentTime >= pump1StartTime || currentTime < pump1StopTime))) {
    pump1Val = 1;
  } else {
    pump1Val = 0;
  }
  if((currentTime >= pump2StartTime && currentTime < pump2StopTime) || (pump2StartTime > pump2StopTime && (currentTime >= pump2StartTime || currentTime < pump2StopTime))) {
    pump2Val = 1;
  } else {
    pump2Val = 0;
  }
}

void buttonCheck(){
  buttonState = digitalRead(buttonPin);  
  buttonStateChange = 0;
 
  if ((millis() - buttonTime) > 400) { 
    if (buttonState == HIGH) { 
      buttonFlag = !buttonFlag;
      buttonTime = millis();
      resetTime = millis();      
      digitalWrite(ledPin, buttonFlag);
      transitionFlag = 0;
    }
    if(buttonState != lastButtonState){
      buttonStateChange = 1;
    }
  }

  if((millis() - resetTime) > buttonActionMillis){
    buttonFlag = 0;
    lastButtonState = 0;
    digitalWrite(ledPin, 0);
  }

  lastButtonState = buttonState;
  }

void buttonFunction(){
  if(buttonFlag){
    if(!transitionFlag){
      if(isDay){  
        transitionDown();
      } else {
        transitionUp();
      }
      transitionFlag = true;
    }
  } else {
    if(!transitionFlag){
      if(isDay){
        transitionUp();     
      } else {
        transitionDown(); 
      }
      transitionFlag = true; 
    }
  }
}

void powerUptransition(){
   for(int i = 0; i <= warmMax; i++){
    analogWrite(warmPin, i);
    delay(5);   //Light Transition Delay
  }
  for(int i = 0; i <= coldMax; i++){
    analogWrite(coldPin, i);
    analogWrite(moonPin, i);
    delay(5);   //Light Transition Delay
  }    
   coldVal = coldMax;
   moonVal = coldMax;
   warmVal = warmMax;   
}

void transitionUp(){
  for(int i = moonMax; i <= coldMax; i++){
    analogWrite(moonPin, i);
    delay(35);   //Moon Light Transition Delay
  }    
  for(int i = 0; i <= warmMax; i++){
    analogWrite(warmPin, i);
    delay(25);   //Warm Light Transition Delay
  }
  for(int i = 0; i <= coldMax; i++){
    analogWrite(coldPin, i);
    delay(15);   //Cold Light Transition Delay
  }  
  coldVal = coldMax;
  moonVal = coldMax;
  warmVal = warmMax;  
}

void transitionDown(){
  for(int i = coldMax; i >= 0; i--){
    analogWrite(coldPin, i);
    delay(30);   //Cold Light Transition Delay
  }   
  for(int i = warmMax; i >= 0; i--){
    analogWrite(warmPin, i);
    delay(20);   //Warm Light Transition Delay
  }   
  for(int i = coldMax; i >= moonMax; i--){
    analogWrite(moonPin, i);
    delay(10);   //Moon Light Transition Delay
  }      
  coldVal = 0;
  moonVal = moonMax;
  warmVal = 0;    
}

void initialize(){
    //Serial.println();  
    Serial.println("====== AQUA TIMER - VER. 3.7S CUBE =====");
    Serial.print("Sunrise: ");
    Serial.print(sunriseHour);
    Serial.print(":");
    Serial.print(sunriseMinutes);
    Serial.println(" h");
    Serial.print("Sundown: ");
    Serial.print(sundownHour);
    Serial.print(":");
    Serial.print(sundownMinutes);
    Serial.println(" h");
    Serial.print("Twilight: ");
    Serial.print(totalTransition);
    Serial.println(" min");    
    Serial.print("Moonlight: ");
    Serial.print(moonlightLength);
    Serial.println(" min");
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");     
    Serial.print("Warm Light: ");
    Serial.print(warmMax);   
    Serial.println("/255");
    Serial.print("Cold Light: ");
    Serial.print(coldMax); 
    Serial.println("/255"); 
    Serial.print("Moon Light: ");
    Serial.print(moonMax);  
    Serial.println("/255");  
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");   
    Serial.print("Pump-1 Start: ");
    Serial.print(pump1StartHour);
    Serial.print(":");
    Serial.print(pump1StartMinutes);
    Serial.println(" h");
    Serial.print("Pump-1 Stop: ");
    Serial.print(pump1StopHour);
    Serial.print(":");
    Serial.print(pump1StopMinutes);
    Serial.println(" h");
    //Serial.print("Pump-1 State: ");
    //Serial.println(pump1Val);
    Serial.println("---------------------");
    Serial.print("Pump-2 Start: ");
    Serial.print(pump2StartHour);
    Serial.print(":");
    Serial.print(pump2StartMinutes);
    Serial.println(" h");
    Serial.print("Pump-2 Stop: ");
    Serial.print(pump2StopHour);
    Serial.print(":");
    Serial.print(pump2StopMinutes);
    Serial.println(" h");
    //Serial.print("Pump-2 State: ");
    //Serial.println(pump2Val);    
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");     
    Serial.print("Button Effect: ");
    Serial.print(buttonActionMin);
    Serial.println(" min");
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");     
    Serial.print("Compile Time: ");
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()){
        Serial.println("RTC Connection Lost !!!");
        Rtc.SetDateTime(compiled);
    }
    if (!Rtc.GetIsRunning()){
        Serial.println("Starting RTC...");
        Rtc.SetIsRunning(true);
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled){
      Serial.println("Updating RTC...");
      Rtc.SetDateTime(compiled);
    }    
    Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low); 

    Serial.print("RTC Time: ");
    printDateTime(now);
    Serial.println();    
    Serial.println("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"); 
    Serial.println("Commands:");
    Serial.println("  'g' - Get Time/Date");
    //Serial.println("  's' - Set Time/Date");   
    Serial.println("  'd' - Is Day Check"); 
    Serial.println("  't' - Transitions Demo");
    Serial.println("=======================================");
    //Serial.println();
}

void getSerialCommand(){
    if(Serial.available()) {
      byte incomingByte = Serial.read();
      //Serial.println(incomingByte);
      
      switch(incomingByte){
        case 's':
          //setRTC();
          Serial.println("Not Active Yet");
          break;

        case 'g':
          getRTC();
          break;

        case 'd':
          Serial.print("Is Day? ");
          Serial.println(isDay);
          break;

        case 't':
          Serial.print("Transition Up In Progress --> ");
          transitionUp();
          Serial.println("Done");
          Serial.print("Transition Down In Progress --> ");
          transitionDown();
          Serial.println("Done");
          break;

        default:
          break;
      }
      clearBuffer();
  }
}

void setRTC(){
  char newTime[10] = {'\0'};
  char newDate[15] = {'\0'};

  clearBuffer();
  
  Serial.println("- Set Time (Dec 31 2000):");
  for(byte i = 0; i < 12; i++){
    serialWait();
    newDate[i] = Serial.read();
  }  
  
  clearBuffer();
  
  Serial.println("- Set Date (23:59:59):");
  for(byte i = 0; i < 9; i++){
    serialWait();
    newTime[i] = Serial.read();
  }
   
  Serial.print("- New Time: ");
  Serial.println(newTime);  
  Serial.print("- New Date: ");
  Serial.println(newDate);  

  clearBuffer();
  
  Serial.println("- Confirm with 'y': ");  
  serialWait();
  if(Serial.read() == 'y'){
    RtcDateTime tmp = RtcDateTime(newDate, newTime);
    Rtc.SetDateTime(tmp);
    Serial.println("- Time Set.");  
  }
}

void getRTC(){
    Serial.print("RTC Time: ");
    printDateTime(now);
    Serial.println();
}

void clearBuffer(){
  for(byte i = 0; i < 65; i++){
    byte c = Serial.read();
  }
}

void serialWait(){
  while(Serial.available() == 0){}
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}
