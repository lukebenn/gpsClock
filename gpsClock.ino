/*
Reqired Componnents:

DS1307 Real Time Clock (I2C)
4 Digit 7 Segment Display (w/Adafruit Backback) (I2C)
4 Digit Alphanumaric Display (w/Adafruit Backback) (I2C)
NEO-6M GPS Module (UART)
Piezo Buzzer (Passive)
Photoresistor
DHT11 Temp and Hum Sensor

Pinouts:

A0 - Photoresistor

A4 - I2C Data
A5 - I2C Clock

D2 - bsc (Big Screen Cycle)
D3 - ssc (Small Screen Cycle)
D4 - A
D5 - B

D8 - Buzzer
D7 - DHT11 Pin
D10- GPS Recive
D11- GPS Transmit (N/C DUE TO 5V Arduino)

*/
//EEPROM SETUP______________________________
#include <EEPROM.h>

//RTC Setup_________________________________
#include <Wire.h>
#include <RTClib.h>
RTC_DS1307 rtc;

//Both LED Displays Setup___________________
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
Adafruit_7segment seg7 = Adafruit_7segment();
Adafruit_AlphaNum4 seg14 = Adafruit_AlphaNum4();

//Temp/Hum Sensor Setup____________________
#include <dht.h>
dht DHT;
#define DHT11_PIN 7
int counter;
double temp;
double newTemp;
double hum;
double newHum;
char cTemp[3]; //Celsius
char cHum[3];  //Relative %
int tempArr[25] = {0};
int humArr[25] = {0};
bool disDF = false; //Display °F  (X°C × 9/5) + 32

//GPS Setup_________________________________
#include <SoftwareSerial.h>
#include <TinyGPS.h>
SoftwareSerial mySerial(10, 11); 
//RX, TX of arduino (reverse of GPS)
//(TX Pin dissconnected when using 5V arduino)
TinyGPS gps;
void gpsdump(TinyGPS &gps);
//void printFloat(double f, int digits = 2);
unsigned long start;
bool gpsFastMode = false;
unsigned long gpsDelay;

//Light Sensor Setup________________________
int lightSensorPin = A0;
int analogValue = 0;

//Global Varibles___________________________
unsigned long age, date, time, chars;
int year;
byte month, day, hour, minute, second, hundredths;
float speed, heading;
int speedUnits = 0;
//0: kph
//1: mph
//2: m/s
//3: knots (Tentative)

//Define Button Pins
#define bsc 2
#define ssc 3
#define a 4
#define b 5

char sec[2];
bool hour24 = false;
bool bscHasReleased = true;
bool sscHasReleased = true;
bool aHasReleased = true;
bool bHasReleased = true;

int bsMode = 1;
//0: Off
//1: Time (HH:MM) (Default)
//2: Date (DD:MM)
//3: Tempurature (XX°C)
//4: Speed (km/h)
//5: Heading (XXX°)
//6: Alarm Set
//7: Time Zone Set

int ssMode = 1;
//0: Off
//1: Seconds (Default)
//2: Date (DD.MM)
//3: Tempurature (XX°C)
//4: Humidity (XXX%)
//5: Speed (km/h)
//6: Heading (XXX°)

//Used for displaying mode titles temporarly when switching modes
int titleDisplay = false;
int titleDisplayTimeout = 0;
int titleDisplayBig = false;
int titleDisplayBigTimeout = 0;
int oldbsMode;  //Undefined so 'TIME' title apprears on small display at boot
int oldssMode = 1;
int oldUnitMode;
int unitDisplay = false;
int unitDisplayTimeout = 0;


//Used for alarm config
#define SNOOZE_MIN 1
int alarmHour = 0;
int alarmMin = 0;
bool alarmEnabled = false;
bool alarmTriggered = false;
bool alarmSnoozed = false;
int snoozeHour;
int snoozeMin;

//Time Zone
int timeZone = -5; //Default EST
bool allowed2Write = false;

//Buzzer Setup
#define BEEP_NOTE 2000
#define BEEP_DURA 64
#define BEEP_PIN 8
//tone(BEEP_PIN, BEEP_NOTE, BEEP_DURA);

//Ignores overflow and unused varible warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Woverflow"
void setup() {
  Serial.begin(9600);   //Set data rate for PC comm port
  mySerial.begin(9600); // Set the data rate for the SoftwareSerial GPS port
  
  seg7.begin(0x70);     //Assigns Address to 7-segment display
  seg14.begin(0x71);    //Assigns Address to 14-segment display

  //Sets up pins used by buttons to inputs
  pinMode(bsc, INPUT);
  pinMode(ssc, INPUT);
  pinMode(a, INPUT);
  pinMode(b, INPUT);
  //rtc.adjust(DateTime(2019, 7, 31, 18, 9, 18)); //Manual Adjust for RTC

  //Disable Onboard LED
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  //EEPROM READING______________________________________________________________
  timeZone = EEPROM.read(1) - 12; 
  alarmHour = EEPROM.read(2);
  alarmMin = EEPROM.read(3);
  Serial.print("Time Zone From EEPROM:  "); Serial.println(timeZone);
  Serial.print("Alarm From EEPROM:  "); Serial.print(alarmHour); Serial.print(":"); Serial.println(alarmMin);
  if(timeZone > 15){
    timeZone = 0;
    EEPROM.write(1, timeZone + 12);
    Serial.print("SET TIMEZONE TO 0");
    delay(500);
  }
  if(alarmHour > 24){
    alarmHour = 0;
    EEPROM.write(2, alarmHour);
    Serial.print("SET ALARM HOUR TO 0");
    delay(500);
  }
  if(alarmMin > 60){
    alarmMin = 0;
    EEPROM.write(3, alarmMin);
    Serial.print("SET ALARM MIN TO 0");
    delay(500);
  }
  
  //CONFIG TEMPURATURE AND HUMIDITY BUFFERS__________________
  for(int k = 0; k < ((sizeof(tempArr)/2)); k++){
    tempArr[k] = -103;
  }
  for(int k = 0; k < ((sizeof(humArr)/2)); k++){
    humArr[k] = -103;
  }

  seg7.clear();
  seg14.clear();

  //Tests all digits of both displays
  seg7.print(8888, DEC);
  seg7.drawColon(true);
  seg14.writeDigitRaw(0, 0xFFFF); seg14.writeDigitRaw(1, 0xFFFF); seg14.writeDigitRaw(2, 0xFFFF); seg14.writeDigitRaw(3, 0xFFFF);
  seg7.writeDigitRaw(2, 0x02 | 0x04 | 0x08 | 0x10);
  
  seg7.writeDisplay();
  seg14.writeDisplay();
  delay(500);

  seg7.clear();
  seg14.clear();
  //Displays Boot Message
  seg7.print(1200, DEC);
  seg7.drawColon(true);
  seg14.writeDigitAscii(0, 'H'); seg14.writeDigitAscii(1, 'e'); seg14.writeDigitRaw(2, 0x36); seg14.writeDigitAscii(3, 'o');  
  //0 DP N M L K J H G2 G1 F E D C B A
  
  seg7.writeDisplay();
  seg14.writeDisplay();
  delay(1000);
  tone(BEEP_PIN, BEEP_NOTE, BEEP_DURA);
  Serial.print("TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.print("\n");
  start = millis();
}

void loop() {
  DateTime UTC = rtc.now(); //Gets RTC time and stores in object
  DateTime now (UTC + TimeSpan(0,timeZone,0,0));
  //Gets Humidity and Temp
  //Counter Exsists due to limitations with sensor polling rate
  //Adjust if statement to increase or decrase update rate
  counter++;
  if (counter == 32766) {
    counter = 0;
  }
  //Checks DHT11 after certain number of cycles
  //Sometimes DHT11 returns an error as a very small int
  //This code ignores that data
  if (counter % 120 == 0) {
    int chk = DHT.read11(DHT11_PIN);
    newTemp = DHT.temperature;
    newHum = DHT.humidity;
    
    if (newTemp > -100){
      //Calculates rolling average tempurature
      //char temperary[60];
      for (int k = (sizeof(tempArr)/2)-1 ; k > 0 ; k--){
        tempArr[k] = tempArr[k-1];
      }
      //Assigns newest temp measurment to begnning of array
      tempArr[0] = newTemp;
      int dataPoints = 0;
      float result = 0;
      for(size_t k = 0; k < (sizeof(tempArr)/2); k++){
        if(tempArr[k] > -99){
          dataPoints++;
          result+=tempArr[k];
        }
      }
      result = result / dataPoints; //Finds rolling average of all tempurature datapoints
      temp = result;
      if (disDF){ //(X°C × 9/5) + 32
        temp = ((temp * 9 / 5 ) + 32);
      }
    }
    if (newHum > -100){
      //Calculates rolling average tempurature
      //char temperary[60];
      for (int k = (sizeof(humArr)/2)-1 ; k > 0 ; k--){
        humArr[k] = humArr[k-1];
      }
      //Assigns newest temp measurment to begnning of array
      humArr[0] = newHum;
      int dataPoints = 0;
      float result = 0;
      for(size_t k = 0; k < (sizeof(humArr)/2); k++){
        if(humArr[k] > -99){
          dataPoints++;
          result+=humArr[k];
        }
      }
      result = result / dataPoints; //Finds rolling average of all tempurature datapoints
      hum = result;
    }
    
    sprintf(cTemp, "%d", (int) temp);
    sprintf(cHum, "%d", (int) hum);
  }
  
  // Every 10 seconds print an update
  // Unless fast mode is enabled (Every Second)
  
  if(gpsFastMode){
    gpsDelay = 1000;
  }
  else{
    gpsDelay = 100000;
  }
  bool newdata = false;
  if((start + gpsDelay) < millis()){
    //Serial.println("FIGHT");
    start = millis();
    while (millis() - start < 600) {
      if (mySerial.available()) {
        char c = mySerial.read();
        //Serial.print(c);  // uncomment to see raw GPS data
        if (gps.encode(c)) {
          newdata = true;
        }
      }
    }
  }
  if (newdata){
    Serial.println("\nAcquired GPS Data");
    //Serial.println("-------------");
    gpsdump(gps);
    rtc.adjust(DateTime(year, month, day, hour, minute, second));  //update rtc with new GPS data
    //Serial.println("-------------");
    //Serial.println();
    char tempNewDate[512];
    sprintf(tempNewDate, "UTC %d.%d.%d \t %d:%d.%d", year, month, day, hour, minute, second);
    Serial.println(tempNewDate);
    
  }

  //Decides Brightness for Displays
  //RANGES from 200ish to about 900
  //Deadzone of 50 to eliminate flicker
  analogValue = analogRead(lightSensorPin);
  if(analogValue > 750){
    seg7.setBrightness(15);
    seg14.setBrightness(12);
  }
  else if(analogValue < 700){
    seg7.setBrightness(3);
    seg14.setBrightness(0);
  }
  
  //For Title display on 14seg for 7seg modes___________________________
  if(oldbsMode != bsMode){
    titleDisplay = true;
    titleDisplayTimeout++;
    //Serial.println("title display TRUE");
  }
  if(titleDisplayTimeout >= 120){
    titleDisplay = false;
    titleDisplayTimeout = 0;
    oldbsMode = bsMode;
    Serial.println("small title display FALSE");
  }

  //For Title display on 7seg for 14seg modes____________________________
  if(oldssMode != ssMode){
    titleDisplayBig = true;
    titleDisplayBigTimeout++;
    //Serial.println("title display TRUE");
  }
  if(titleDisplayBigTimeout >= 120){
    titleDisplayBig = false;
    titleDisplayBigTimeout = 0;
    oldssMode = ssMode;
    Serial.println("big title display FALSE");
  }

  //For Speed Units display______________________________________________
  if(oldUnitMode != speedUnits){
    unitDisplay = true;
    unitDisplayTimeout++;
    //Serial.println("title display TRUE");
  }
  if(unitDisplayTimeout >= 120){
    unitDisplay = false;
    unitDisplayTimeout = 0;
    oldUnitMode = speedUnits;
    Serial.println("unit display FALSE");
  }
  
  //Blinks display if in alarm or time zone set mode
  if(bsMode == 6 || bsMode == 7)
    seg7.blinkRate(1);
  else
    seg7.blinkRate(0);

  //EEPROM SAVING______________________________________________________________
  //1: timeZone
  //2: alarmHour
  //3: alarmMin
  if(bsMode != 6 && bsMode != 7){
      if(EEPROM.read(1) != timeZone && allowed2Write){
        EEPROM.write(1, timeZone + 12);
        Serial.print("Wrote Time Zone: ");
        Serial.println(timeZone);
        allowed2Write = false;
        delay(600);
      }
      else if(EEPROM.read(2) != alarmHour){
        EEPROM.write(2, alarmHour);
        Serial.println("Wrote Alarm Hour");
        delay(600);
      }
      else if(EEPROM.read(3) != alarmMin){
        Serial.print("Wrote Alarm Minute: EEPROM: ");
        Serial.print(EEPROM.read(3));
        Serial.print(" Variable: ");
        Serial.println(alarmMin);
        EEPROM.write(3, alarmMin);
        delay(600);
      }
  }
  
  //BUTTON PROCCESSING_________________________________________________________
  //Storing Button States for this cycle
  int bscState = digitalRead(bsc);
  int sscState = digitalRead(ssc);
  int aButState = digitalRead(a);
  int bButState = digitalRead(b);
  //Uses Two Buttons to cycle both top and bottom displays seperatly
  if (bscState == HIGH && bscHasReleased && !alarmTriggered && !alarmSnoozed) {
    tone(BEEP_PIN, BEEP_NOTE, BEEP_DURA);
    delay(BEEP_DURA);
    //Serial.print("BSC");
    bscHasReleased = false;
    if (bsMode >= 7)
      bsMode = 0;
    else
      bsMode++;
      titleDisplayTimeout = 0;
      oldbsMode = 100;
  }
  if (bscState == LOW){
    bscHasReleased = true;
  }

  if (sscState == HIGH && sscHasReleased && !alarmTriggered && !alarmSnoozed) {
    tone(BEEP_PIN, BEEP_NOTE, BEEP_DURA);
    delay(BEEP_DURA);
    //Serial.print("SSC");
    sscHasReleased = false;
    if (ssMode >= 6)
      ssMode = 0;
    else
      ssMode++;
      titleDisplayBigTimeout = 0;
      oldssMode = 100;
  }
  if (sscState == LOW){
    sscHasReleased = true;
  }

  if (aButState == HIGH && aHasReleased && !alarmTriggered && !alarmSnoozed) {
    delay(BEEP_DURA);
    aHasReleased = false;
    if (bsMode == 1){
      if (hour24){
        hour24 = false;
        tone(BEEP_PIN, 1500, BEEP_DURA);
      }
      else if (!hour24){
        hour24 = true;
        tone(BEEP_PIN, 2200, BEEP_DURA);
      }
    }
    if (bsMode == 3){
      if (disDF){
        disDF = false;
        tone(BEEP_PIN, 1500, BEEP_DURA);
      }
      else if (!disDF){
        disDF = true;
        tone(BEEP_PIN, 2200, BEEP_DURA);
      }
    }
    //Heading or Speed
    if (bsMode == 4 || bsMode == 5){
      //gpsFastMode = !gpsFastMode;
      if (gpsFastMode){
        gpsFastMode = false;
        tone(BEEP_PIN, 1500, BEEP_DURA);
      }
      else if (!gpsFastMode){
        gpsFastMode = true;
        tone(BEEP_PIN, 2200, BEEP_DURA);
      }
    }
    if (bsMode == 6){
      tone(BEEP_PIN, BEEP_NOTE, BEEP_DURA);
      alarmHour++;
      if(alarmHour >= 24)
        alarmHour = 0;
    }
    else if (bsMode == 7){
      if(timeZone > -12){
        tone(BEEP_PIN, 1500, BEEP_DURA);
        timeZone--;
        allowed2Write = true;
      }
    }
  }
  if (aButState == LOW){
    aHasReleased = true;
  }
  
  if (bButState == HIGH && bHasReleased && !alarmTriggered) {
    bHasReleased = false;
    if (bsMode == 1){
       if (alarmEnabled){
        alarmEnabled = false;
        tone(BEEP_PIN, 1500, BEEP_DURA);
      }
      else if (!alarmEnabled){
        alarmEnabled = true;
        tone(BEEP_PIN, 2200, BEEP_DURA);
      }
    }
    else if (bsMode == 4){
      tone(BEEP_PIN, BEEP_NOTE, BEEP_DURA);
      speedUnits++;
      unitDisplayTimeout = 0;
      oldUnitMode = 100;
      if(speedUnits > 3)
        speedUnits = 0;
    }
    else if (bsMode == 6){
      tone(BEEP_PIN, BEEP_NOTE, BEEP_DURA);
      alarmMin++;
      if(alarmMin >= 60)
        alarmMin = 0;
    }
    else if (bsMode == 7){
      if(timeZone < 14){
        tone(BEEP_PIN, 2200, BEEP_DURA);
        timeZone++;
        allowed2Write = true;
      }
    }
  }
  if (bButState == LOW){
    bHasReleased = true;
  }

  //Alarm Calculations_________________________________________________________
  
  //Triggers alarm if minute and hour is equal to current time
  if(!alarmEnabled && (alarmSnoozed || alarmTriggered)){
    alarmTriggered = false;
    alarmSnoozed = false;
    Serial.println("Alarm Disabled");
  }
  if(now.hour() == alarmHour && now.minute() == alarmMin && !alarmSnoozed && alarmEnabled && !alarmTriggered){
    alarmTriggered = true;
    Serial.println("ALARM TRIGGERED");
  }

  else if((now.hour() == snoozeHour && now.minute() == snoozeMin) && alarmSnoozed && alarmEnabled){
    alarmTriggered = true;
    alarmSnoozed = false;
    Serial.println("SNOOZE ALARM TRIGGERED");
  }

  if (alarmTriggered){
    bsMode = 1; //SETS BIG DISPLAY TO TIME MODE
    tone(BEEP_PIN, BEEP_NOTE, 64);
    delay(128);
    tone(BEEP_PIN, BEEP_NOTE, 64);
    delay(128);
    tone(BEEP_PIN, BEEP_NOTE, 64);
    delay(128);
    tone(BEEP_PIN, BEEP_NOTE, 64);
    delay(512);
    if(bscState == HIGH || sscState == HIGH || aButState == HIGH || bButState == HIGH){
      alarmSnoozed = true;
      alarmTriggered = false;
      //Prevents Accidental button presses
      sscHasReleased = false;
      bscHasReleased = false;
      aHasReleased = false;
      bHasReleased = false; 
      tone(BEEP_PIN, 2500, 256);
      delay(500);
      DateTime snoozeObj (now + TimeSpan(0,0,SNOOZE_MIN,0));
      snoozeHour = snoozeObj.hour();
      snoozeMin = snoozeObj.minute();
      Serial.print("Snooze Time: "); Serial.print(snoozeHour); Serial.print(":"); Serial.println(snoozeMin);
    }
  }
  
  //Defining Each Mode
  //
  //BIG SCREEN MENUS__________________________________________________________
  //
  if(titleDisplayBig == false){
    switch (bsMode) {
      //Display Off
      case 0:{
        seg7.clear();
        break;
      }
      //Time
      case 1:{
        int cMin = now.minute();
        int cHour = now.hour();
        if (cHour >= 12 && ! hour24) {
          if (cHour > 12){
            cHour = cHour - 12;
          }
        }
        else if (cHour == 0 && ! hour24){
          cHour = 12;
        }
        else if (cHour == 0 && hour24){
          cHour = 0;
        }
        seg7.print((cHour * 100 + cMin), DEC);

        if(cHour == 0 && hour24){
          seg7.writeDigitRaw(1,0b00111111);// Draws 0 symbol (_gfedcba)
          if(cMin < 10)
            seg7.writeDigitRaw(3,0b00111111);
        }

        //Colon Calculation Stuff

        int Ccol = 0x00;
        int Ucol = 0x00;
        int Lcol = 0x00;
        int Dcol = 0x00;
        int result;

        if(now.second() % 2 == 0) Ccol = 0x02;
        if(alarmEnabled)          Lcol = 0x08;
        if(!hour24)               Ucol = 0x04;
        if(alarmSnoozed)          Dcol = 0x10;

        result = Ccol | Ucol | Lcol | Dcol;
        seg7.writeDigitRaw(2, result);
        
        break;
      }
      //Date
      case 2:{
        seg7.print((now.month() * 100 + now.day()), DEC);
        seg7.drawColon(false);
        break;
      }
      //Tempurature
      case 3:{
        seg7.print((int) temp * 100, DEC);
        seg7.writeDigitRaw(3,0b01100011);// Draws ° symbol (_gfedcba)
        if(disDF)
          seg7.writeDigitRaw(4,0b01110001);//Removes 0 from far right segment
        else
          seg7.writeDigitRaw(4,0b00111001);
        seg7.drawColon(false);
        break;
      }
      //Speed
      case 4:{
        seg7.print((int) speed, DEC);
        if(gpsFastMode)
          seg7.writeDigitRaw(2, 0x04);
        break;
      }
      //Heading
      case 5:{
        seg7.print((int) heading, DEC);
        if(gpsFastMode)
          seg7.writeDigitRaw(2, 0x04);
        break;
      }
      //Alarm Set
      case 6:{
        int cMin = alarmMin;
        int cHour = alarmHour;
        if (cHour >= 12 && ! hour24) {
          if (cHour > 12){
            cHour = cHour - 12;
          }
        }
        else if (cHour == 0 && ! hour24){
          cHour = 12;
        }
        else if (cHour == 0 && hour24){
          cHour = 0;
        }
        seg7.print((cHour * 100 + cMin), DEC);
        
        //seg7.drawColon(true);

        if (alarmHour >= 12 && !hour24) 
          seg7.writeDigitRaw(2, 0x04 | 0x02); //(2, 0x08, true);
        else  
          seg7.writeDigitRaw(2, 0x02);

        if(alarmHour == 0 && hour24){
          seg7.writeDigitRaw(1,0b00111111);// Draws 0 symbol (_gfedcba)
          if(cMin < 10)
            seg7.writeDigitRaw(3,0b00111111);
        }
        break;
      }
      //Time Zone Adjust
      case 7:{
          seg7.print(timeZone, DEC);
          break;
        }
      default:{
        seg7.clear();
        Serial.println("An error occured in bsMode case");
        bsMode = 0;
        break;
      }
    }
  }
  //
  //Small Screen Menus_________________________________________________________
  //
  if(titleDisplay == false){
    seg14.clear();
    switch (ssMode) {
      //Display Off
      case 0:{
        seg14.clear();
        break;
      }
      //Time (Seconds)
      case 1:{
        int cSec = now.second();
        sprintf(sec, "%d", cSec); //Converts seconds into format for seg14
        if (cSec > 9) {
          seg14.writeDigitAscii(2, sec[0]);
          seg14.writeDigitAscii(3, sec[1]);
        }
        else {
          seg14.writeDigitAscii(2, '0');
          seg14.writeDigitAscii(3, sec[0]);
        }
        break;
      }
      //Date
      case 2:{
        seg14.clear();
        char cDay[3];
        char cMonth[3];
        sprintf(cDay, "%d", (int) now.day());
        sprintf(cMonth, "%d", (int) now.month());

        if (now.month() > 9) {
          seg14.writeDigitAscii(0, cMonth[0]);
          seg14.writeDigitAscii(1, cMonth[1], true);
        }
        else {
          //seg14.writeDigitAscii(2, '0');
          seg14.writeDigitAscii(1, cMonth[0], true);
        }

        if (now.day() > 9) {
          seg14.writeDigitAscii(2, cDay[0]);
          seg14.writeDigitAscii(3, cDay[1]);
        }
        else {
          //seg14.writeDigitAscii(2, '0');
          seg14.writeDigitAscii(3, cDay[0]);
        }
        break;
      }
      //Tempurature
      case 3:{
        seg14.clear();
        if(temp >= 10 || temp <= -10){
        seg14.writeDigitAscii(0, cTemp[0]);
        seg14.writeDigitAscii(1, cTemp[1]);
        }
        else if(temp <= 9 && temp > -9){
          seg14.writeDigitAscii(1, cTemp[0]);
        }
        seg14.writeDigitRaw(2, 0x00E3);

        if(disDF)
          seg14.writeDigitAscii(3, 'F');
        else
          seg14.writeDigitAscii(3, 'C');
        break;
      }
      //Humidity
      case 4:{
        if(hum >= 10 || hum <= -10){
        seg14.writeDigitAscii(1, cHum[0]);
        seg14.writeDigitAscii(2, cHum[1]);
        }
        else if(hum <= 9 && hum > -9){
          seg14.writeDigitAscii(2, cHum[0]);
        }
        seg14.writeDigitAscii(3, '%');
        break;
      }
      //Speed
      case 5:{
        seg14.clear();
        char cSpeed[16];
        char cSpeed2[16];
        sprintf(cSpeed2, "%d", (int) speed);
        dtostrf(speed, 2, 1, cSpeed);
        //Serial.print(cSpeed);
        //Serial.print("   ");
        //Serial.println(speed);
        
        if(speed != 0){
          if(speed >= 100){
            seg14.writeDigitAscii(0, cSpeed2[0]);
            seg14.writeDigitAscii(1, cSpeed2[1]);
            seg14.writeDigitAscii(2, cSpeed2[2], true);
          }
          else if(speed >= 10){
            seg14.writeDigitAscii(1, cSpeed2[0]);
            seg14.writeDigitAscii(2, cSpeed2[1], true);
          }
          else{
            seg14.writeDigitAscii(2, cSpeed2[0], true);
          }
          seg14.writeDigitAscii(3, cSpeed[2]);
        }
        else{
          seg14.writeDigitAscii(2, '0', true);
          seg14.writeDigitAscii(3, '0');
        }
        break;
      }
      //Heading
      case 6:{
        seg14.clear();
        seg14.writeDigitRaw(3, 0x00E3);
        char cHead[16];
        sprintf(cHead, "%d", (int) heading);

        if(heading >= 100){
            seg14.writeDigitAscii(0, cHead[0]);
            seg14.writeDigitAscii(1, cHead[1]);
            seg14.writeDigitAscii(2, cHead[2]);
          }
          else if(heading >= 10){
            seg14.writeDigitAscii(1, cHead[0]);
            seg14.writeDigitAscii(2, cHead[1]);
          }
          else if(heading != 0){
            seg14.writeDigitAscii(2, cHead[0]);
          }
          else{
            seg14.writeDigitAscii(0, '-'); seg14.writeDigitAscii(1, '-'); seg14.writeDigitAscii(2, '-');
          }
        //Serial.println("HEADING 14seg");
        break;
      }
      default:{
        seg14.clear();
        Serial.println("An error occured in ssMode case, switched to SSC0");
        ssMode = 0;
        break;
      }
    }
  }

  //Title Displays_________________________________________________________________________________________
  //overides 14seg display to display mode title
  if(titleDisplay == true){
    switch (bsMode) {
      //Display Off
      case 0:{
        seg14.writeDigitAscii(0, ' '); 
        seg14.writeDigitAscii(1, 'O'); 
        seg14.writeDigitAscii(2, 'F'); 
        seg14.writeDigitAscii(3, 'F');
        break;
      }
      //Time
      case 1:{
        seg14.writeDigitAscii(0, 'T'); 
        seg14.writeDigitAscii(1, 'I'); 
        seg14.writeDigitAscii(2, 'M'); 
        seg14.writeDigitAscii(3, 'E');
        break;
      }
      //Date
      case 2:{
        seg14.writeDigitAscii(0, 'D'); 
        seg14.writeDigitAscii(1, 'A'); 
        seg14.writeDigitAscii(2, 'T'); 
        seg14.writeDigitAscii(3, 'E');
        break;
      }
      //Tempurature
      case 3:{
        seg14.writeDigitAscii(0, 'T'); 
        seg14.writeDigitAscii(1, 'E'); 
        seg14.writeDigitAscii(2, 'M'); 
        seg14.writeDigitAscii(3, 'P');
        break;
      }
      //Speed
      case 4:{
        seg14.writeDigitAscii(0, ' '); 
        seg14.writeDigitAscii(1, 'S'); 
        seg14.writeDigitAscii(2, 'P'); 
        seg14.writeDigitAscii(3, 'D');
        break;
      }
      //Heading
      case 5:{
        seg14.writeDigitAscii(0, ' '); 
        seg14.writeDigitAscii(1, 'H'); 
        seg14.writeDigitAscii(2, 'D'); 
        seg14.writeDigitAscii(3, 'G');
        break;
      }
      //Alarm Set Mode
      case 6:{
        seg14.writeDigitAscii(0, 'A', true); 
        seg14.writeDigitAscii(1, 'S'); 
        seg14.writeDigitAscii(2, 'e'); 
        seg14.writeDigitAscii(3, 't');
        break;
      }
      //Time Zone Set
      case 7:{
        seg14.writeDigitAscii(0, 'T', true); 
        seg14.writeDigitAscii(1, 'Z'); 
        seg14.writeDigitAscii(2, 'n'); 
        seg14.writeDigitAscii(3, 'e');
        break;
      }
      default:{
        seg14.clear();
        break;
      }
    }
  }
  
  if(titleDisplayBig == true){
    switch (ssMode) {
      //Display Off
      case 0:{
        seg7.clear();
        seg7.print(0x0FF,HEX);
        seg7.writeDigitRaw(1,0b00111111);// Draws 0 symbol (_gfedcba)
        break;
      }
      //Time (SECONDS)
      case 1:{
        //T I M E
        seg7.clear();
        seg7.writeDigitRaw(0,0b01111000);// Draws T symbol (_gfedcba)
        seg7.writeDigitRaw(1,0b00110000);// Draws I symbol (_gfedcba)
        seg7.writeDigitRaw(3,0b00010101);// Draws M symbol (_gfedcba)
        seg7.writeDigitRaw(4,0b01111001);// Draws E symbol (_gfedcba)
        break;
      }
      //Date
      case 2:{
        //d A t E
        seg7.clear();
        seg7.writeDigitRaw(0,0b01011110);// Draws D symbol (_gfedcba)
        seg7.writeDigitRaw(1,0b01110111);// Draws A symbol (_gfedcba)
        seg7.writeDigitRaw(3,0b01111000);// Draws T symbol (_gfedcba)
        seg7.writeDigitRaw(4,0b01111001);// Draws E symbol (_gfedcba)
        break;
      }
      //Tempurature (°C)
      case 3:{
        //t E M P 
        seg7.clear();
        seg7.writeDigitRaw(0,0b01111000);// Draws T symbol (_gfedcba)
        seg7.writeDigitRaw(1,0b01111001);// Draws E symbol (_gfedcba)
        seg7.writeDigitRaw(3,0b00010101);// Draws M symbol (_gfedcba)
        seg7.writeDigitRaw(4,0b01110011);// Draws P symbol (_gfedcba)
        break;
      }
      //Humidity
      case 4:{
        //H U M
        seg7.clear();
        seg7.writeDigitRaw(1,0b01110110);// Draws H symbol (_gfedcba)
        seg7.writeDigitRaw(3,0b00111110);// Draws U symbol (_gfedcba)
        seg7.writeDigitRaw(4,0b00010101);// Draws M symbol (_gfedcba)
        break;
      }
      //Speed
      case 5:{
        //S P d
        seg7.clear();
        seg7.writeDigitRaw(1,0b01101101);// Draws S symbol (_gfedcba)
        seg7.writeDigitRaw(3,0b01110011);// Draws P symbol (_gfedcba)
        seg7.writeDigitRaw(4,0b01011110);// Draws D symbol (_gfedcba)
        break;
      }
      //Heading
      case 6:{
        seg7.clear();
        seg7.writeDigitRaw(1,0b01110110);// Draws H symbol (_gfedcba)
        seg7.writeDigitRaw(3,0b01011110);// Draws D symbol (_gfedcba)
        seg7.writeDigitRaw(4,0b00111101);// Draws G symbol (_gfedcba)
        break;
      }
      default:{
        seg7.clear();
        break;
      }
    }
  }

  //Overides 14seg display to display units
  if(unitDisplay == true){
    switch (speedUnits) {
      //Display Off
      case 0:{
        seg14.writeDigitAscii(0, 'K'); 
        seg14.writeDigitAscii(1, 'm'); 
        seg14.writeDigitAscii(2, '/'); 
        seg14.writeDigitAscii(3, 'h');
        break;
      }
      //Time
      case 1:{
        seg14.writeDigitAscii(0, ' '); 
        seg14.writeDigitAscii(1, 'm'); 
        seg14.writeDigitAscii(2, 'p'); 
        seg14.writeDigitAscii(3, 'h');
        break;
      }
      //Date
      case 2:{
        seg14.writeDigitAscii(0, ' '); 
        seg14.writeDigitAscii(1, 'M'); 
        seg14.writeDigitAscii(2, '/'); 
        seg14.writeDigitAscii(3, 's');
        break;
      }
      //Tempurature
      case 3:{
        seg14.writeDigitAscii(0, 'K'); 
        seg14.writeDigitAscii(1, 'n'); 
        seg14.writeDigitAscii(2, 't'); 
        seg14.writeDigitAscii(3, 's');
        break;
      }
      default:{
        seg14.clear();
        break;
      }
    }
  }
  //Update Displays
  seg7.writeDisplay();
  seg14.writeDisplay();
}

//Aquires and stores required GPS Data
//Time, Speed, Heading
//Also decides what speed unit to display given the 'speedUnits' variable
void gpsdump(TinyGPS &gps) {
  gps.get_datetime(&date, &time, &age);
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);

  heading = gps.f_course();

  //Decides which speed unit to display given 'speedUnits' variable
  switch (speedUnits) {
    case 0: speed = gps.f_speed_kmph();
      break;
    case 1: speed = gps.f_speed_mph();
      break;
    case 2: speed = gps.f_speed_mps();
      break;
    case 3: speed = gps.f_speed_knots();
      break;
    default: speed = gps.f_speed_kmph();
      break;
  }
}

void printnvram(uint8_t address) {
  Serial.print("Address 0x");
  Serial.print(address, HEX);
  Serial.print(" = 0x");
  Serial.println(rtc.readnvram(address), HEX);
}
#pragma GCC diagnostic pop
