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

#include "icons.h"
// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 480 + 336)
const int epd_bitmap_allArray_LEN = 5;
const unsigned char *epd_bitmap_allArray[5] = { // 32x23px
    epd_bitmap_check_lg,
    epd_bitmap_exclamation_octagon_fill,
    epd_bitmap_pause_fill,
    epd_bitmap_stopwatch,
    epd_bitmap_x_lg};

const unsigned char *getIcon(const char *icon)
{
  if (strcmp(icon, "error") == 0)
  {
    return epd_bitmap_allArray[1];
  }
  else if (strcmp(icon, "pause") == 0)
  {
    return epd_bitmap_allArray[2];
  }
  else if (strcmp(icon, "stopwatch") == 0)
  {
    return epd_bitmap_allArray[3];
  }
  else if (strcmp(icon, "check") == 0)
  {
    return epd_bitmap_allArray[0];
  }
  else if (strcmp(icon, "x") == 0)
  {
    return epd_bitmap_allArray[4];
  }
  else
  {
    return epd_bitmap_allArray[4];
  }
}

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

void setup()
{

  RTC.begin();
  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Arduino Arcade Time");
  display.drawBitmap(0, 7, getIcon("stopwatch"), 32, 23, WHITE);
  display.display();

  delay(5000);
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Communication with WiFi module failed!");
    display.display();
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

  String dot = "";

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Connecting to");
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

  http_request();
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
  // Bars
  RTCTime currentTime;

  RTC.getTime(currentTime);
  display.clearDisplay();
  printWifiBar();
  display.setCursor(0, 0);

  String hours = String(currentTime.getHour() + timeZoneOffsetHours);
  String minutes = String(currentTime.getMinutes());
  String seconds = String(currentTime.getSeconds());
  display.print(hours.length() == 1 ? "0" + hours : hours);
  display.print(":");
  display.print(minutes.length() == 1 ? "0" + minutes : minutes);
  display.print(":");
  display.print(seconds.length() == 1 ? "0" + seconds : seconds);
  // Body

  if (jsonResult.hasOwnProperty("ok"))
  {
    if ((bool)jsonResult["ok"] == true)
    {
      JSONVar data = jsonResult["data"];

      if (data["completed"])
      {
        display.setCursor(36, 10);
        display.print("No session");
        display.drawBitmap(0, 10, getIcon("x"), 32, 23, WHITE);
      }
      else if ((bool)data["paused"])
      {
        display.setCursor(36, 10);
        display.print("Paused");
        display.drawBitmap(0, 10, getIcon("pause"), 32, 23, WHITE);
      }
      else
      {
        long remaining = long(data["endTime"]) - currentTime.getUnixTime();
        int minutesR = remaining / 60;
        int secondsR = remaining % 60;

        if (secondsR < 0)
        {
          display.setCursor(36, 10);
          display.print("Session ended");
          display.drawBitmap(0, 10, getIcon("check"), 32, 23, WHITE);
        }
        else
        {

          char remainingMinStr[3];
          char remainingSecStr[3];
          sprintf(remainingMinStr, "%02d", minutesR);
          sprintf(remainingSecStr, "%02d", secondsR);

          display.setCursor(36, 10);
          display.print(remainingMinStr);
          display.print(":");
          display.print(remainingSecStr);
        }
        display.drawBitmap(0, 10, getIcon("stopwatch"), 32, 23, WHITE);
      }
    }
    else
    {
      display.setCursor(36, 14);
      display.print("No data");
      display.drawBitmap(0, 10, getIcon("error"), 32, 23, WHITE);
    }
  }

  display.display();

  read_response();
  if (millis() - lastConnectionTime > postingInterval)
  {
    http_request();
  }
}

void read_response()
{
  if (client.available())
  {
    Serial.println("reading response");
    String line = client.readStringUntil('\r');

    Serial.print(line);
    if (line == "\n")
    {
      Serial.println("headers received");
      String payload = client.readString();
      myObject = JSON.parse(payload);
      jsonResult = myObject;
      Serial.println(jsonResult);
    }
  }
}

void http_request()
{

  client.stop();

  Serial.println("\nStarting connection to server...");
  if (client.connect(host, 80))
  {
    Serial.println("connected to server");
    String request = "/api/session/" + String(slackid);
    client.println("GET " + request + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Authorization: Bearer " + String(apiKey));
    client.println("Connection: close");
    client.println();
    lastConnectionTime = millis();
  }
  else
  {
    Serial.println("connection failed");
  }
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
