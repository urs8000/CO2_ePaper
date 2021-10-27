
// TTGO-T5 V2.0 2.13inch ePaper Display
// board: ESP32 Arduino --> ESP32 Dev Board

// Reference: https://github.com/Xinyuan-LilyGO/T5-Ink-Screen-Series

// !! original !!
// Sketch uses 1628374 bytes (124%) of program storage space. Maximum is 1310720 bytes.
// Global variables use 91660 bytes (27%) of dynamic memory, leaving 236020 bytes for local variables. Maximum is 327680 bytes.
// does this help??? https://github.com/schreibfaul1/ESP32-MiniWebRadio/issues/9
// it works but not with 1.6MB !!!

// !! WiFi, NTP, ThingSpeak removed !!
// Sketch uses 1234238 bytes (94%) of program storage space. Maximum is 1310720 bytes.
// Global variables use 70384 bytes (21%) of dynamic memory, leaving 257296 bytes for local variables. Maximum is 327680 bytes.

// Sensor:   Sensirion SCD30             (Mouser 403-SCD30 (47.--))
// ePaper:   TTGO-T5-V2.3  2.13inch ePaper   (LILYGO® TTGO T5 V2.3 2.13 Inch E-Paper Screen New Driver Chip)
//           https://www.aliexpress.com/item/32869729970.html   13.13 + 6.77 = 19.90  (10 Stück = 145.--)
//           https://github.com/lewisxhe/TTGO-EPaper-Series
// test:     GxEPD2_Example.ino  select the below used definition out of the bunch of possible
//
// // select below the correct Gx-display-library
// board with    LEDs is B73
// board without LEDs is B74
// 
// 
// open item (--):   **  solved: (++):
// -- implement forced calibration
// -- NTP returns strange date. it should be verified and reloaded (full refresh)
// ++ wake-up not tested yet -> changed to timer-sleep <- removed. running this steady powered
// -- battery usage not tested yet
// -- SCD30 does not sleep (20-75mA power usage) --> implement setMeasurementInterval(uint16_t interval)
// -- long term usage not tested yet.  possible to attach a solar cell loading the LiPo?
//    sometimes it hangs on 0ppm --> reboot after some sensor reads with 0ppm??
// -- data storage & external access(--> RasPi, Influx, Grafana) https://www.reichelt.de/magazin/how-to/sensordatenbank-auf-dem-raspberry-pi/
// -- Sensirion SPS30 particulate matter sensor
// 
// -- other sensors should be added e.g. SPS30 Particle Sensor (Mouser 403-SPS30 (39.--)), SVM30-J Air Quality Sensor with SGP30 and SHTC1(Mouser 403-SVM30-J (20.--))
// -- case  https://www.thingiverse.com/search?q=TTGO+T5&type=things&sort=relevant
// -- board for attaching the sensors to make it stable

// ERROR:
// !! If SCD30 is initialized LED19 does NOT light on/off (but the message arrives)
//
// ** 20201018:  restructuring and splitting into functions
// ** sprintf see: http://www2.hs-esslingen.de/~zomotor/home.fhtw-berlin.de/junghans/cref/FUNCTIONS/format.html
// 
// 20211003   add BLE connection to Sensirion myAmbience App
// 20211003   sketch is too big. removing WiFi, NTP, Thinspeak.  so ONLY BLE connection is possible
/* compiled size
   with debug output
     Sketch uses 1234242 bytes (94%) of program storage space. Maximum is 1310720 bytes.
     Global variables use 70392 bytes (21%) of dynamic memory, leaving 257288 bytes for local variables. Maximum is 327680 bytes.
   no debug output
     Sketch uses 1233222 bytes (94%) of program storage space. Maximum is 1310720 bytes.
     Global variables use 70392 bytes (21%) of dynamic memory, leaving 257288 bytes for local variables. Maximum is 327680 bytes.
*/
/*  
 ********************* ONLY LOCAL VERSION.   No PubSub No BM6xx ********************************************************************
*/


//Includes
#include <GxEPD2_BW.h>                                   // https://github.com/ZinggJM/GxEPD2  use GxEPD2_Example an entry point
#include <U8g2_for_Adafruit_GFX.h>
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>              // 
#include <Adafruit_NeoPixel.h>                           // 
#include "Sensirion_GadgetBle_Lib.h"                     // Sensirion Lib https://github.com/Sensirion/arduino-ble-gadget



//Defines
#define ANALOGPIN               35                       // Battery Voltage
#define MINVOLT               1830.0                     // 3.3*4096/6.6
#define MAXVOLT               2330.0                     // 4.2*4096/6.6
#define MAXNROFATTEMPTS          8                       // 
#define uS_TO_S_FACTOR     1000000                       // Conversion factor for micro seconds to seconds
#define TIME_TO_BOOT            30                       // on error (no wifi, no sensors) reboot to probably fix it
#define LoopUpdateDelay      15000                       // 15 seconds delay between measurements and BLE update
#define LoopMultiplier           1                       // 
#define DisplayUpdateMultiplier  4                       // no need to update the ePaper from each measurement
                                                         // update after 60 seconds
#define Neopi_pin               12                       // 
#define NUMPIXELS                1                       // Popular NeoPixel ring size
#define NPixDelay               10                       // 10 msec delay between NPixel

//Global variables
const int BUILTIN_LED = 19;                              // for TTGO-T5 V2.3 (GREEN)
unsigned long previousMillis  =      0;                  // 
int DisplayUpdateCnt          =      1;                  //
bool FirstDisplayWrite        =   true;                  // no wait until display update loop finished - immediate write after boot
bool keepDisplay_on           =  false;                  // overwrite display update pause
String ErrorText;                                        // max size = 23 characters
int BRIGHTNESS = 20;                                     // 50 is very bright

// Global Sensor Data
int   scd30_co2_val;                                     // CO2 from Sensirion SCD30 CO2-Sensor
float scd30_temp_val;                                    // temperature  "
float scd30_humi_val;                                    // humidity     "
float batt_val;                                          // battery
char  scd30_co2Char[9];                                  // 1457ppm
char  scd30_tempChar[8];                                 // 99.9°C
char  scd30_humiChar[8];                                 // 99.9%RF
char  batteryChar[5];                                    // 
String BLE_Device_ID;                                    // show device-id xx:xx on diplay


//Define services
// board B73
GxEPD2_BW<GxEPD2_213_B73, GxEPD2_213_B73::HEIGHT> display(GxEPD2_213_B73(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));    // GDEH0213B73 <--- use THIS ! see GX-library
// board B74
// GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B73::HEIGHT> display(GxEPD2_213_B74(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEH0213B74 -> LILYGO® TTGO T5 V2.3.1_2.13 Inch E-Paper Screen New Driver Chip GDEM0213B74
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;                            // 
SCD30 airSensor;                                            // address 0x61
Adafruit_NeoPixel pixels(NUMPIXELS, Neopi_pin, NEO_GRB + NEO_KHZ800);
GadgetBle gadgetBle = GadgetBle(GadgetBle::DataType::T_RH_CO2_ALT);

// enable debug statements to Serial Monitor if uncommented
#define my_DEBUG                             // un-/comment that statement in your program to enable/disable debugging output
// Debugging included 
#ifdef my_DEBUG
#define DEBUG_WRT(...) { Serial.write(__VA_ARGS__); }
#define DEBUG_PRT(...) { Serial.print(__VA_ARGS__); }
#define DEBUG_PLN(...) { Serial.println(__VA_ARGS__); }
#define DEBUG_BEGIN()  { Serial.begin(57600); }
#else
#define DEBUG_WRT(...) {}
#define DEBUG_PRT(...) {}
#define DEBUG_PLN(...) {}
#define DEBUG_BEGIN()  {}
#endif



void blink_RED(int count)                                    // expand for other external LEDs
{
  for (int i = 0; i != count; i++) {
    digitalWrite(BUILTIN_LED, HIGH);    delay(50);
    digitalWrite(BUILTIN_LED, LOW);     delay(50);
   }
}

void reboot()
{
   esp_sleep_enable_timer_wakeup(TIME_TO_BOOT * uS_TO_S_FACTOR);    // wakeup via built-in timer sleep for xx seconds, then reboot after wakeup
   esp_deep_sleep_start();                                          // do so
}

void readSensor() {                                              // insert here all sensor communication and make their results public
  int maxRetry = 10;                                             // as many times each sensor could be read until ZERO will be filled in each value
  int retryCnt =  0;
  
  DEBUG_PLN("Sensor data reading ...");
  batt_val = BatteryLevel();                                     // get Battery-Level from Analog
  
  if ((airSensor.dataAvailable()) && (retryCnt != maxRetry)) {                               // 
    scd30_co2_val  = airSensor.getCO2();                             // read Sensor
    scd30_temp_val = airSensor.getTemperature();                     // 
    scd30_humi_val = airSensor.getHumidity();                        // 
   } else {
    retryCnt++; 
    DEBUG_PRT(" SCD30 retry: "); 
    DEBUG_PLN(retryCnt);
    delay(2000);
    if (retryCnt == maxRetry) reboot();                              // it seems to hang somewhere
   }
   retryCnt = 0;
   DEBUG_PRT("SCD30 CO2:  ");   DEBUG_PLN(scd30_co2_val);
   DEBUG_PRT("SCD30 Temp: ");   DEBUG_PLN(scd30_temp_val);
   DEBUG_PRT("SCD30 Humi: ");   DEBUG_PLN(scd30_humi_val);
   DEBUG_PLN();
}   

void led_ALL_off() {
     pixels.clear();  delay(100);
     DEBUG_PLN("all off");
     delay(2000);
}
void led_GREEN_on() {
  for(int i=0; i<NUMPIXELS; i++) {
     pixels.setPixelColor(i, pixels.Color(0, 254, 0));
     pixels.show();
     delay(NPixDelay);
  }   
     DEBUG_PLN("GREEN on");
}
void led_YELLOW_on() {
  for(int i=0; i<NUMPIXELS; i++) {
     pixels.setPixelColor(i, pixels.Color(254, 90, 0));
     pixels.show();
     delay(NPixDelay);
  }   
     DEBUG_PLN("YELLOW on");
}
void led_RED_on() {
  for(int i=0; i<NUMPIXELS; i++) {
     pixels.setPixelColor(i, pixels.Color(254, 0, 0));
     pixels.show();
     delay(NPixDelay);
  }
     DEBUG_PLN("RED on");
}
void led_BLUE_on() {
  for(int i=0; i<NUMPIXELS; i++) {
     pixels.setPixelColor(i, pixels.Color(0, 0, 250));
     pixels.show();
     delay(NPixDelay);
  }
     DEBUG_PLN("BLUE on");
}

void action_on_values() {                                      // add here all local action based on sensor data
  int switch_val;
   
  led_ALL_off();
  if  (scd30_co2_val <700)   switch_val = 1;
  if ((scd30_co2_val >701) && 
      (scd30_co2_val <1000)) switch_val = 2;
  if  (scd30_co2_val >1001)  switch_val = 3;
  switch (switch_val) {
    case 1:
      led_GREEN_on();
      break;
    case 2:
      led_YELLOW_on();
      break;
    case 3:
      led_RED_on();
      keepDisplay_on = true;                                   // always update values on display if they are in RED area
      break;  
    default:
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------
void setup()                                     // 
{
  DEBUG_BEGIN();
  pinMode(BUILTIN_LED, OUTPUT);                  // initialize all used GPIOs
  pixels.begin();                                // initialize NeoPixel object
  delay(100);                                    // 
  pixels.setBrightness(50);  
  led_ALL_off();
  led_BLUE_on();
  delay(100);

  display.init();                                // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  u8g2Fonts.begin(display);                      //
  display.clearScreen();                         // clear display

  Wire.begin();    delay(100);
                                                 // initiate here all attached sensors
  if (airSensor.begin() == false) {              // 
    ErrorText = "Sensor not available";
    DrawError(ErrorText);
    delay(500);
    while(1);                                    // REBOOT HERE TOO ??????????????????????????????????
  }
  airSensor.setAltitudeCompensation(430);        // Set altitude of the sensor in m

  gadgetBle.begin();                             // initialize BLE connection
  BLE_Device_ID = gadgetBle.getDeviceIdString();
  DEBUG_PRT("Sensirion GadgetBle Lib initialized with deviceId = ");         // no error handling implemented yet !
  DEBUG_PLN(gadgetBle.getDeviceIdString());
  
  DEBUG_PLN("Setup done");
  led_ALL_off(); pixels.clear(); pixels.show(); delay(100);
  
  readSensor();
  DrawText();
}


void loop()                                            // 
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= (LoopUpdateDelay * LoopMultiplier)) {   
    previousMillis = currentMillis;                    // save the last time the inside loop wasexecuted

    readSensor();                                      // get all data from the sensors and
 // transferData();                                    // transfer them to the external devices (ThingSpeak, IFTTT, MQTT, ... )
    action_on_values();                                // 
    gadgetBle.writeTemperature(scd30_temp_val);
    gadgetBle.writeHumidity(scd30_humi_val);
    gadgetBle.writeCO2(scd30_co2_val);
    gadgetBle.commit();

    gadgetBle.handleEvents();

    if ((DisplayUpdateCnt == DisplayUpdateMultiplier)  // display update each x loop
     || (FirstDisplayWrite == true)                    //                after boot
     || (keepDisplay_on    == true))                   //                if values must be shown
      {
//      DateTime2String();                             // 0-128-00
//      BatteryLevel();                                // 
        DrawText();                                    // show data on ePaper, then
        display.hibernate();                           // 
        FirstDisplayWrite = false;                     // 
        keepDisplay_on    = false;                     // would be set after next measurement
        DisplayUpdateCnt  = 0;                         // 
        DEBUG_PLN("Display update");                   // 
      }
    DisplayUpdateCnt++;                                // 
  } 
}
//----------------------------------------------------------------------------------------------------------------------------------------------

void DrawError(String ErrorText)                                      // something went wrong
{
  display.setRotation(1);                             // 
  u8g2Fonts.setFontMode(1);                           // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);                      // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);          // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);          // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_logisoso16_tf);         // was 58 but too big for xxxppm  46 ->42
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  u8g2Fonts.setCursor(10, 100);                       // start writing at this position   tempx, tempy
  u8g2Fonts.print(ErrorText);                         // Errortext to display
  display.display(false);
}

void DrawText()
{
  dtostrf(scd30_co2_val, 4, 0, scd30_co2Char);        // 
  strcat(scd30_co2Char, "ppm");                       // 

  dtostrf(scd30_temp_val, 3, 1, scd30_tempChar);      // 
  strcat(scd30_tempChar, "°C");                       // 
  dtostrf(scd30_humi_val, 3, 1, scd30_humiChar);      // 
  strcat(scd30_humiChar, "%RF");                      // 

  dtostrf(batt_val, 3, 0, batteryChar);               //
  strcat(batteryChar, "%");                           // 
  
  display.setRotation(1);                             // 
  u8g2Fonts.setFontMode(1);                           // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);                      // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);          // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);          // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_logisoso42_tf);         // was 58 but too big for xxxppm  46 ->42
  uint16_t tempx = (display.width() - 210) / 2;       // full size is 213 x 73    was 200
  uint16_t tempy = display.height() - 45;             // was 20
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  u8g2Fonts.setCursor(tempx, tempy);                  // start writing at this position
  u8g2Fonts.print(scd30_co2Char);                     // CO2 value
  u8g2Fonts.setFont(u8g2_font_logisoso16_tf);         
  u8g2Fonts.setCursor(10, 26);                        // start writing at this position
//  u8g2Fonts.print(dateChar);                          // 0-250-00 is WRONG time detected
  u8g2Fonts.print("Device-ID: ");
  u8g2Fonts.print(BLE_Device_ID);
  u8g2Fonts.setCursor(120, 26);                       // start writing at this position
//  u8g2Fonts.print(timeChar);
  u8g2Fonts.setCursor(210, 26);                       // start writing at this position
  u8g2Fonts.print(batteryChar);

  u8g2Fonts.setCursor(60, 120);                       // start writing at this position
  u8g2Fonts.print(scd30_tempChar);                    // 
  u8g2Fonts.setCursor(140, 120);                      // start writing at this position
  u8g2Fonts.print(scd30_humiChar);                    // 

  display.display(false);
}


// ---------------------------------------------------------------------------------------------------

float BatteryLevel()
{
  float batteryPercent;
  unsigned int batteryData = 0;

  for (int i = 0; i < 64; i++) {
    batteryData = batteryData + analogRead(ANALOGPIN);
  }
  batteryData = batteryData >> 6; //divide by 64
  batteryPercent = (float(batteryData) - MINVOLT) / (MAXVOLT - MINVOLT) * 100.0;
  if (batteryPercent < 0.0) {
    batteryPercent = 0.0;
  }
  if (batteryPercent > 100.0) {
    batteryPercent = 100.0;
  }
  return batteryPercent;
}
