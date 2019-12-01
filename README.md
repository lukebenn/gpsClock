# gpsClock
Arduino-based GPS desk clock loaded with additional features  
  
  
## Official Feature List
 - Dual Display  
 - 12/24 Hour Clock 
 - Auto-Adjusted time by GPS  
 - Single Alarm with snooze function
 - °C Tempurature Display  
 - % Humidity Display  
 - Manually Adjusted Time Zone  
 - Speed and Heading Displays with ~1 seccond update rate (Using Fast Mode)  
 - Date Display (MM.DD)  
 - Mode Display for knowing current display page  
 - *Fast Mode* increases GPS update rate with conprimise of slower button response 
 (Activated with A Button on 7 Segment Speed Display)
  
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
   
   
## Arduino Pinout
**A0** - Photoresistor  
  
**A4** - I2C Data  
**A5** - I2C Clock  
  
**D2** - bsc (Big Screen Cycle)  
**D3** - ssc (Small Screen Cycle)  
**D4** - A  
**D5** - B  
  
**D8** - Buzzer  
**D7** - DHT11 Pin  
**D10** - GPS Recive  
**D11** - GPS Transmit (N/C on 5V Arduinos)  

### Improvments
 - Moving to a 3.3V Arduino and a 5V Supply for displays will allow the ability to increase bause rate of GPS and further increase update rate without the need for fast mode.  
 - More Precise Tempurature Gauge  
