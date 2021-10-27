
// TTGO-T5 V2.0 2.13inch ePaper Display
// board: ESP32 Arduino --> ESP32 Dev Board

// select below the correct Gx-display-library
// board with    LEDs is B73
// board without LEDs is B74


//Includes
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <NTPtimeESP.h>
#include <HTTPClient.h>
#include <WiFiCred.h>
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h"              // 


//Defines
#define TIMEOUT   5000                                   // 5 sec
#define ANALOGPIN 35
#define MINVOLT 1830.0                                   // 3.3*4096/6.6
#define MAXVOLT 2330.0                                   // 4.2*4096/6.6
#define OFFSET 8.0
#define MAXNROFATTEMPTS 8

//Global variables
char dateChar[13];
char timeChar[11];
strDateTime dateTime;

//Define services
//GxEPD2_BW<GxEPD2_213_B73, GxEPD2_213_B73::HEIGHT> display(GxEPD2_213_B73(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEH0213B73
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B73::HEIGHT> display(GxEPD2_213_B74(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEH0213B74 -> LILYGO® TTGO T5 V2.3.1_2.13 Inch E-Paper Screen New Driver Chip GDEM0213B74
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
NTPtime NTPhu("ch.pool.ntp.org");                           // Choose server pool as required
WiFiClient wclient;                                         //
HTTPClient hclient;                                         // 
SCD30 airSensor;                                            // S I M U L A T E D VALUES !!!!


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


void setup()
{
  unsigned long wifitimeout = millis();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);             // 1 = High, 0 = Low
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    if (millis() - wifitimeout > TIMEOUT)
     {
       break;
     }
   }
  display.init();                            // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  u8g2Fonts.begin(display);
  display.clearScreen(GxEPD_WHITE);
  DateTime2String();                         // 
  BatteryLevel();                            // 
  DrawText();                                // 
  display.hibernate();                       // 
  esp_deep_sleep_start();                    // 
}

void loop()
{
}

void DrawText()
{
  int   co2_value;                            // CO2 from Sensirion SCD30 CO2-Sensor
  float temp_value;                           // temperature  "
  float humi_value;                           // humidity     "
  float batt_value;                           // battery
  char co2Char[9];                            // 1457ppm
  char tempChar[8];                           // 99.9°C
  char humiChar[8];                           // 99.9%RF
  char batteryChar[5];                        // 

  batt_value = BatteryLevel();                // get Battery-Level from Analog
  co2_value  = 1447;      // airSensor.getCO2();                             // read Sensor
  temp_value = 34.7;      // airSensor.getTemperature();                     // 
  humi_value = 57.2;      // airSensor.getHumidity();                        // 
  
  dtostrf(co2_value, 4, 0, co2Char);          // 
  strcat(co2Char, "ppm");                     // 

  dtostrf(temp_value, 3, 1, tempChar);        // 
  strcat(tempChar, "°C");                     // 
  dtostrf(humi_value, 3, 1, humiChar);        // 
  strcat(humiChar, "%RF");                    // 

  dtostrf(batt_value, 3, 0, batteryChar);     //
  strcat(batteryChar, "%");                   // 
  
  display.setRotation(1);                             // 
  u8g2Fonts.setFontMode(1);                           // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);                      // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);          // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);          // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_logisoso42_tf);         // was 58 but too big for xxxppm  46 ->42
  uint16_t tempx = (display.width() - 210) / 2;       // 213 x 73    was 200
  uint16_t tempy = display.height() - 45;             // was 20
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  u8g2Fonts.setCursor(tempx, tempy);                  // start writing at this position
  u8g2Fonts.print(co2Char);                           // CO2 value
  u8g2Fonts.setFont(u8g2_font_logisoso16_tf);
  u8g2Fonts.setCursor(10, 26);                        // start writing at this position
  u8g2Fonts.print(dateChar);                          // 0-250-00 is WRONG
  u8g2Fonts.setCursor(120, 26);                       // start writing at this position
  u8g2Fonts.print(timeChar);
  u8g2Fonts.setCursor(210, 26);                       // start writing at this position
  u8g2Fonts.print(batteryChar);

  u8g2Fonts.setCursor(60, 120);                       // start writing at this position
  u8g2Fonts.print(tempChar);                          // 
  u8g2Fonts.setCursor(140, 120);                      // start writing at this position
  u8g2Fonts.print(humiChar);                          // 

  display.display(false);

}


// ---------------------------------------------------------------------------------------------------


bool RefreshDateTime()
{
  dateTime = NTPhu.getNTPtime(1.0, 1);
  if (dateTime.year > 2035)
  {
    return 0;
  }
  return (dateTime.valid);
}

void DateTime2String()
{
  int nroftry = 0;
  while (!RefreshDateTime())
  {
    if (++nroftry > 1000)
    {
      break;
    }
  }
  sprintf(dateChar, "%i-%02i-%02i", dateTime.year, dateTime.month, dateTime.day);
  sprintf(timeChar, "%02i:%02i:%02i", dateTime.hour, dateTime.minute, dateTime.second);
}

/*
float ReadTransmitter()
{
  float transData;
  int nrOfattempts = 0;
  bool successComm = 0;

  while ((!successComm) && (nrOfattempts < MAXNROFATTEMPTS))
  {
    hclient.begin(wclient, host);
    hclient.setConnectTimeout(500);
    if (HTTP_CODE_OK == hclient.GET())
    {
      successComm = 1;
      transData = hclient.getString().toFloat();
      transData -= OFFSET;
    }
    else
    {
      successComm = 0;
      nrOfattempts ++;
      transData = 0.0f;
    }
    hclient.end();
  }

  if ((transData < 10.0) || transData > 84.0)
  {
    transData = 0.0f;
  }
  return transData;
}
*/


float BatteryLevel()
{
  float batteryPercent;
  unsigned int batteryData = 0;

  for (int i = 0; i < 64; i++)
  {
    batteryData = batteryData + analogRead(ANALOGPIN);
  }
  batteryData = batteryData >> 6; //divide by 64
  batteryPercent = (float(batteryData) - MINVOLT) / (MAXVOLT - MINVOLT) * 100.0;
  if (batteryPercent < 0.0)
  {
    batteryPercent = 0.0;
  }
  if (batteryPercent > 100.0)
  {
    batteryPercent = 100.0;
  }
  return batteryPercent;
}
