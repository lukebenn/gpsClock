# gpsClock
Arduino-based GPS desk clock loaded with additional features  
  
  
## Official Feature List
 - Dual Display  (Large 4-Dig 7-Seg, Small 4-Dig 14-Seg)  
 - 12/24 Hour Clock 
 - Auto-Adjusted time by GPS  
 - Single Alarm with snooze function
 - °C Tempurature Display  
 - % Humidity Display  
 - Manually Adjusted Time Zone 
 - Auto Brightness
 - Speed and Heading Displays with ~1 seccond update rate (Using Fast Mode)  
 - Date Display (MM.DD)  
 - Mode Display for knowing current display page  
 - *Fast Mode* increases GPS update rate with comprimise of slower button response 
 (Activated with A Button on 7 Segment Speed or Heading Display)
  
## Required Hardware
 - Arduino Device (Only Tested on UNO and NANO)  
 - 4 Active-High Buttons  
 - Piezo  
 - Photoressitor  
 - DHT11 Tempurature Sensor (Serial)  
 - NEO-6M GPS Module (UART)  
 - DS1307 Real Time Clock Module (I2C)
 - 4-Digit 14-Segment Display with Adafruit Backpack (I2C Address set to 0x71)  
 - 4-Digit 7-Segment Display with Adafruit Backpack (I2C Address set to 0x70, default)  
 
## Required Libraries
 - Wire.h (Included with Arduino IDE)
 - EEPROM.h (Included with Arduino IDE)
 - SoftwareSerial.h
 - dht.h
 - RTClib.h
 - Adafruit_GFX.h
 - Adafruit_LEDBackpack.h
 - TinyGPS.h
   
## Arduino Pinout
**A0** - Photoresistor  
  
**A4** - I2C Data  
**A5** - I2C Clock  
  
**D2** - B  
**D3** - A  
**D4** - ssc (Small Screen Cycle)  
**D5** - bsc (Big Screen Cycle)  
  
**D8** - Buzzer  
**D7** - DHT11 Serial Data Pin  
**D10** - GPS RX (Recive)
**D11** - GPS TX (Transmit) (N/C on 5V Arduinos)  

## How to Read Schematic File
 1. Download Fritzing
 2. Install Adafruit LED Backpack Parts
 3. Open .fzz file

![Schematic](/schem/schem.png)

### Improvments
 - Moving to a 3.3V Arduino and a 5V Supply for displays will allow the ability to increase bause rate of GPS and further increase update rate without the need for fast mode.  
 - More Precise Tempurature Gauge DHT22 is great but more precision would be great.
