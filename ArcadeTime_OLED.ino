#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "RTC.h"
#include <Wire.h>
#include <NTPClient.h>

ArduinoLEDMatrix matrix;

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);


#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <Arduino_JSON.h>

const char *host = "hackhour.hackclub.com";
#include "arduino_secrets.h"

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp;
NTPClient timeClient(Udp);

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 10000;
JSONVar myObject;
JSONVar jsonResult;

auto timeZoneOffsetHours = 2;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
char *apiKey = SECRET_API_KEY;
char *slackid = SECRET_SLACK_ID;
int keyIndex = 0; // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiClient client;

/* -------------------------------------------------------------------------- */
void setup()
{

  RTC.begin();
  /* -------------------------------------------------------------------------- */
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Arduino Arcade Time");
  display.display();

  delay(5000);
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Communication with WiFi module failed!");
    display.display();
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Please upgrade the firmware");
    display.display();
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Attempting to connect to SSID:");
    display.setCursor(0, 1);
    display.print(ssid);
    display.display();
    status = WiFi.begin(ssid, pass);
  }

  printWifiStatus();
  delay(5000);
  Serial.println("\nStarting connection to server...");
  timeClient.begin();
  timeClient.update();

  auto unixTime = timeClient.getEpochTime();
  Serial.print("Unix time = ");
  Serial.println(unixTime);
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);

  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.println("The RTC was just set to: " + String(currentTime));

}

/*
EXAMPLE RESPONSE
{
    "ok": true,
    "data": {
        "id": "slackId",
        "createdAt": "2024-06-23T02:49:17.900Z",
        "time": 60,
        "elapsed": 12,
        "remaining": 48,
        "endTime": "2024-06-23T03:08:00.000Z",
        "goal": "No Goal",
        "paused": true,
        "completed": false,
        "messageTs": "messageTs",
    }
}
*/

void loop()
{



}


void printWifiBar()
{
  long rssi = WiFi.RSSI();

  if (rssi > -55)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.fillRect(117, 2, 4, 6, WHITE);
    display.fillRect(122, 0, 4, 8, WHITE);
  }
  else if (rssi<-55 & rssi> - 65)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.fillRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
  else if (rssi<-65 & rssi> - 75)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
  else if (rssi<-75 & rssi> - 85)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
  else if (rssi<-85 & rssi> - 96)
  {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.drawRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
  else
  {
    display.drawRect(102, 7, 4, 1, WHITE);
    display.drawRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
}

void printWifiStatus()
{

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(String(WiFi.SSID()));

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  display.setCursor(0, 20);
  display.println(ip);

  long rssi = WiFi.RSSI();

  printWifiBar();

  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  display.setCursor(0, 10);
  display.print(String(rssi) + " dBm");
  display.display();
}
