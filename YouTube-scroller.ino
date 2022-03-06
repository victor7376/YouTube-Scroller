//==========================================
//  Difference between ESP32 and ESP8266
//  ESP32 is more powerful with faster WiFi
//==========================================
//default libraries
#include <SPI.h>
#include <Adafruit_GFX.h> // --> https://github.com/adafruit/Adafruit-GFX-Library
#include <Max72xxPanel.h> // --> https://github.com/markruys/arduino-Max72xxPanel
#include "TimeDB.h"
#include <YoutubeApi.h>
#include <WiFiClientSecure.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

//============================ Your WIFI Network Details ==============================
const char* ssid     = "";          // Your SSID / WiFi Network
const char* password = "";  // Your SSID / WiFi Password
//=====================================================================================

//================================ TimezoneDB Settings ================================
String TIMEDBKEY = ""; // Your API Key from https://timezonedb.com/register

// Location taaken from TimezoneDB - can be taken from the homepage
String LAT = "";          // Latitude
String LON = "";          // Longitude
//=========================== YouTube Details ========================================
boolean YOUTUBE_ENABLED = false; // Change to true to be enabled
String CHANNEL_ID = "";
String API_KEY = "";
//=====================================================================================
/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

// Display Settings
// CLK -> D5 (SCK)  
// CS  -> D6 
// DIN -> D7 (MOSI)

const int pinCS = D6; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )

// Here you can choose how many displays you have - Displays come in a set of 4, so double would be 8 etc
const int numberOfHorizontalDisplays = 4; // default 4 for standard 4 x 1 display Max size of 24 works
const int numberOfVerticalDisplays = 1; // default 1 for a single row height

int displayIntensity = 1;  //(This can be set from 0 - 15) - how bright you want the display
int minutesBetweenDataRefresh = 15;  // Time in minutes between data refresh (default 15 minutes)
int minutesBetweenScrolling = 3; // Time in minutes between scrolling data (default 1 minutes and max is 10)
int displayScrollSpeed = 25; // In milliseconds -- Configurable by the web UI (slow = 35, normal = 25, fast = 15, very fast = 5)
boolean flashOnSeconds = true; // when true the : character in the time will flash on and off as a seconds indicator
boolean IS_24HOUR = true; // 23:00 millitary 24 hour clock
boolean IS_PM = true; // Show PM indicator on Clock when in AM/PM mode
int ledRotation = 3;
String timeDisplayTurnsOn = "08:30";  // 24 Hour Format HH:MM -- Leave blank for always on. (ie 05:30)
String timeDisplayTurnsOff = "22:30"; // 24 Hour Format HH:MM -- Leave blank for always on. Both must be set to work.
String marqueeMessage = "";           // Custom Message if wanted

// LED Settings
const int offset = 1;
int refresh = 0;
String message = "hi";
int spacer = 1;  // dots between letters
int width = 5 + spacer; // The font width is 5 pixels + spacer
String Wide_Clock_Style = "3";
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

// TimeDB - do NOT alter
TimeDB TimeDB("");
String lastMinute = "xx";
int displayRefreshCount = 1;
long lastEpoch = 0;
long firstEpoch = 0;
long displayOffEpoch = 0;
boolean displayOn = true;
// OTA
boolean ENABLE_OTA = true;    // this will allow you to load firmware to the device over WiFi (see OTA for ESP8266)
String OTA_Password = "";     // Set an OTA password here -- leave blank if you don't want to be prompted for password
#define HOSTNAME "YTube-"

// YouTube
WiFiClientSecure client;
YoutubeApi api(API_KEY, client);


// YouTube Rounding
float roundTowardZero(float number, int decimal_place) {
    float round_number = round(number);
    if(round_number > number) round_number = round_number - 1;
    float deci = (number - round_number) * pow(10, decimal_place);
    float round_decimal = round(deci);
    if(round_decimal > deci) round_decimal = round_decimal - 1;
    round_decimal = round_decimal / pow(10, decimal_place);
    float total_number = round_number + round_decimal;
    return total_number;
}

// Main program
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10);
 //New Line to clear from start garbage
  Serial.println();

  client.setInsecure();
  
  Serial.println();
  Serial.println();
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println();

  ArduinoOTA.setHostname(HOSTNAME);
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  
  Serial.println();
  Serial.println("Number of LED Displays: " + String(numberOfHorizontalDisplays));
  // initialize dispaly
  matrix.setIntensity(0); // Use a value between 0 and 15 for brightness

  int maxPos = numberOfHorizontalDisplays * numberOfVerticalDisplays;
  for (int i = 0; i < maxPos; i++) {
    matrix.setRotation(i, ledRotation);
    matrix.setPosition(i, maxPos - i - 1, 0);
  }

  Serial.println("matrix created");
  matrix.fillScreen(LOW); // show black
  centerPrint("- YTube -");
  for (int inx = 0; inx <= 8; inx++) {
    matrix.setIntensity(inx);
    delay(100);
  }
  for (int inx = 15; inx >= 0; inx--) {
    matrix.setIntensity(inx);
    delay(60);
  }
  delay(1000);
  matrix.setIntensity(displayIntensity);
}

// Now for the loop which will run and run....
void loop() {
  // put your main code here, to run repeatedly:

  float subs;
  String subs_mod;
  if(api.getChannelStatistics(CHANNEL_ID)){
     // float subs = 8888888;
     subs = api.channelStats.subscriberCount; 
     
     if (subs > 999 && subs <= 9999 ) {
        float num = subs / 1000;
        float noround = roundTowardZero(num, 3);
        
        subs_mod = String(noround, 2) + "K";
     
     }  else if (subs > 9999 && subs<=99999) {
        float num = subs / 1000;
        float noround = roundTowardZero(num, 1);
        
        subs_mod = String(noround, 1) + "K";
     
     }  else if (subs > 99999 && subs <= 999999 ) { // up to 1M
        float num = subs / 1000;
        float noround = roundTowardZero(num, 0);
        
        subs_mod = String(noround, 0) + "K";
     
     }  else if (subs > 999999 && subs <= 9999999) { // up to 10M
        float num = subs / 1000000;
        float noround = roundTowardZero(num, 2);
        
        subs_mod = String(noround, 2) + "M";
        
     }  else if (subs > 9999999 && subs <= 99999999) { // up to 100M
        float num = subs / 1000000;
        float noround = roundTowardZero(num, 1);
        
        subs_mod = String(noround, 1) + "M";
        
     }  else if (subs > 99999999 ) { // over 100M
        float num = subs / 1000000;
        float noround = roundTowardZero(num, 0);
        
        subs_mod = String(noround, 0) + "M";
        
     } else {
      
      subs_mod = String(subs,0);
     }
  }
  ArduinoOTA.handle();
  // Get some Time Data to serve
  if ((getMinutesFromLastRefresh() >= minutesBetweenDataRefresh) || lastEpoch == 0) {
    getTimeData();
  }
  checkDisplay(); // this will see if we need to turn it on or off for night mode.

  if (lastMinute != TimeDB.zeroPad(minute())) {
    lastMinute = TimeDB.zeroPad(minute());

    matrix.fillScreen(LOW); // show black
    
    displayRefreshCount --;
    // Check to see if we need to Scroll some Data
    if (displayRefreshCount <= 0) {
      displayRefreshCount = minutesBetweenScrolling;
      String msg;
      msg += " ";

      msg += TimeDB.getDayName() + ", " + day();
      // Sort out the dates if you can.
      if (day() == 1 || day() == 21 || day() == 31) {
      msg += "st, ";
      } else if (day() == 2 || day() == 22) {
      msg += "nd, ";
      } else if (day() == 3 || day() == 23) {
      msg += "rd, ";
      } else {
      msg += "th, ";
      }
        
      msg += TimeDB.getMonthName() + ", " + year() +" :";  //  moved day to previous line, added comma after month name, added year display and colon
      
      msg += marqueeMessage + " "; // custom message

      if (YOUTUBE_ENABLED == true) {
                msg += " YouTube Stats:";
        
       if (subs >= 1) {
          msg += "You have: " + subs_mod + " Subscribers ";
        }
      } // end of YT section
      
    msg += marqueeMessage + " ";
        
    msg += ": Next Update: " + getTimeTillUpdate() + " ";
      
    scrollMessage(msg);
    }    
  }

  String currentTime = hourMinutes(false);

// ================================================
// Clock Styles are in this section
// ================================================
    if (numberOfHorizontalDisplays >= 8) {

    // Time plus seconds no flashing colon
    if (Wide_Clock_Style == "1") {
      currentTime += secondsIndicator(false) + TimeDB.zeroPad(second());
      matrix.fillScreen(LOW); // show black
    }
    // Normal clock
    if (Wide_Clock_Style == "2") {
      // No change this is normal clock display
    }
    
    if (Wide_Clock_Style == "4") {
      String timeSpacer = " ";
      currentTime += secondsIndicator(true) + TimeDB.zeroPad(second()) + timeSpacer + day() + "/" + month() + "/" +year();
      
      matrix.fillScreen(LOW); // show black
    }
  }
// ================================================
// End of Clock Styles section
// ================================================  

  matrix.fillScreen(LOW);
  centerPrint(currentTime, true);
}

void scrollMessage(String msg) {
  msg += " "; // add a space at the end
  for ( int i = 0 ; i < width * msg.length() + matrix.width() - 1 - spacer; i++ ) {
    
    if (refresh == 1) i = 0;
    refresh = 0;
    matrix.fillScreen(LOW);

    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically

    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < msg.length() ) {
        matrix.drawChar(x, y, msg[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrix.write(); // Send bitmap to display
    delay(displayScrollSpeed);
  }
  matrix.setCursor(0, 0);
}

void centerPrint(String msg) {
  centerPrint(msg, true);
}

void centerPrint(String msg, boolean extraStuff) {
  int x = (matrix.width() - (msg.length() * width)) / 2;

  // Print the static portions of the display before the main Message
  if (extraStuff) {
    if (!IS_24HOUR && IS_PM && isPM()) {
      matrix.drawPixel(matrix.width() - 1, 6, HIGH);
    }
  }
  matrix.setCursor(x, 0);
  matrix.print(msg);
  matrix.write();
}

String secondsIndicator(boolean isRefresh) {
  String rtnValue = ":";
  if (isRefresh == false && (flashOnSeconds && (second() % 2) == 0)) {
    rtnValue = " ";
  }
  return rtnValue;
}

String hourMinutes(boolean isRefresh) {
  if (IS_24HOUR) {
    return String(hour()) + secondsIndicator(isRefresh) + TimeDB.zeroPad(minute());
  } else {
    return String(hourFormat12()) + secondsIndicator(isRefresh) + TimeDB.zeroPad(minute());
  }
}

void getTimeData() //client function to send/receive GET request data.
{
  matrix.fillScreen(LOW); // show black
  Serial.println();

  if (displayOn) {
    // only pull the weather data if display is on
    if (firstEpoch != 0) {
      centerPrint(hourMinutes(true), true);
    } else {
      centerPrint("Updating");
    }
      matrix.drawPixel(0, 7, HIGH);
      matrix.drawPixel(0, 6, HIGH);
      matrix.drawPixel(0, 5, HIGH);
      matrix.write();
    }
  Serial.println("Updating Time...");
  //Update the Time
  matrix.drawPixel(0, 4, HIGH);
  matrix.drawPixel(0, 3, HIGH);
  matrix.drawPixel(0, 2, HIGH);
  Serial.println("matrix Width:" + matrix.width());
  matrix.write();
  TimeDB.updateConfig(TIMEDBKEY, LAT, LON);
  time_t currentTime = TimeDB.getTime();
  if(currentTime > 5000 || firstEpoch == 0) {
    setTime(currentTime);
  } else {
    Serial.println("Time update unsuccessful!");
  }
  lastEpoch = now();
  if (firstEpoch == 0) {
    firstEpoch = now();
    Serial.println("firstEpoch is: " + String(firstEpoch));
  }

  Serial.println();
}

int getMinutesFromLastRefresh() {
  int minutes = (now() - lastEpoch) / 60;
  return minutes;
}

int getMinutesFromLastDisplay() {
  int minutes = (now() - displayOffEpoch) / 60;
  return minutes;
}

String getTimeTillUpdate() {
  String rtnValue = "";

  long timeToUpdate = (((minutesBetweenDataRefresh * 60) + lastEpoch) - now());

  int hours = numberOfHours(timeToUpdate);
  int minutes = numberOfMinutes(timeToUpdate);
  int seconds = numberOfSeconds(timeToUpdate);

  rtnValue += String(hours) + ":";
  if (minutes < 10) {
    rtnValue += "0";
  }
  rtnValue += String(minutes) + ":";
  if (seconds < 10) {
    rtnValue += "0";
  }
  rtnValue += String(seconds);

  return rtnValue;
}

void checkDisplay() {
  if (timeDisplayTurnsOn == "" || timeDisplayTurnsOff == "") {
    return; // nothing to do
  }
  String currentTime = TimeDB.zeroPad(hour()) + ":" + TimeDB.zeroPad(minute());

  if (currentTime == timeDisplayTurnsOn && !displayOn) {
    Serial.println("Time to turn display on: " + currentTime);
    enableDisplay(true);
  }

  if (currentTime == timeDisplayTurnsOff && displayOn) {
    Serial.println("Time to turn display off: " + currentTime);
    enableDisplay(false);
  }
}

void enableDisplay(boolean enable) {
  displayOn = enable;
  if (enable) {
    if (getMinutesFromLastDisplay() >= minutesBetweenDataRefresh) {
      // The display has been off longer than the minutes between refresh -- need to get fresh data
      lastEpoch = 0; // this should force a data pull of the weather
      displayOffEpoch = 0;  // reset
    }
    matrix.shutdown(false);
    matrix.fillScreen(LOW); // show black
    Serial.println("Display was turned ON: " + now());
  } else {
    matrix.shutdown(true);
    Serial.println("Display was turned OFF: " + now());
    displayOffEpoch = lastEpoch;
  }
}
