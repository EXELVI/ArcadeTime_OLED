#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <DS3231.h>
#include <Wire.h>
#include <NTPClient.h>
#include <TimeLib.h>

auto timeZoneOffsetHours = 2; // Time zone offset in hours from UTC

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET); // Create an instance of the display

#include "icons.h" // Include the icons
// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 480 + 336)
const int epd_bitmap_allArray_LEN = 5;
const unsigned char *epd_bitmap_allArray[8] = { // 32x23px
    epd_bitmap_check_lg,
    epd_bitmap_exclamation_octagon_fill,
    epd_bitmap_pause_fill,
    epd_bitmap_stopwatch,
    epd_bitmap_x_lg,
    epd_bitmap_hourglass_bottom,
    epd_bitmap_hourglass_split,
    epd_bitmap_hourglass_top};

const unsigned char *getIcon(const char *icon) // Function to get the icon
{
  if (strcmp(icon, "error") == 0) // strcmp means string compare, 0 when equal
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
  else if (strcmp(icon, "hourglass_bottom") == 0)
  {
    return epd_bitmap_allArray[5];
  }
  else if (strcmp(icon, "hourglass_split") == 0)
  {
    return epd_bitmap_allArray[6];
  }
  else if (strcmp(icon, "hourglass_top") == 0)
  {
    return epd_bitmap_allArray[7];
  }
  else
  {
    return epd_bitmap_allArray[4]; // Default to x icon
  }
}

#include <WiFiNINA.h>
#include <WiFiUdp.h>      // Include the WiFiUdp library for NTP
#include <Arduino_JSON.h> // Include the Arduino_JSON library

const char *host = "hackhour.hackclub.com"; // Host
#include "arduino_secrets.h"                // Include the secrets file

DS3231 clock;

int wifiStatus = WL_IDLE_STATUS; // WiFi status
WiFiUDP Udp;                     // Create an instance of the WiFiUDP class
NTPClient timeClient(Udp);       // Create an instance of the NTPClient class

unsigned long lastConnectionTime = 0;        // Last connection time
const unsigned long postingInterval = 10000; // Posting interval
JSONVar jsonResult;                          // Create an instance of the JSONVar class

char ssid[] = SECRET_SSID;       // SSID (edit in arduino_secrets.h)
char pass[] = SECRET_PASS;       // Password (edit in arduino_secrets.h)
char *apiKey = SECRET_API_KEY;   // API key (edit in arduino_secrets.h)
char *slackid = SECRET_SLACK_ID; // Slack ID (edit in arduino_secrets.h)
int keyIndex = 0;                // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS; // WiFi status

WiFiClient client; // Create an instance of the WiFiClient class

float parseISO8601(const char *datetime)
{
  tmElements_t tm;
  int year, month, day, hour, minute, second;

  sscanf(datetime, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);

  tm.Year = year - 1970; // Anni dal 1970
  tm.Month = month;
  tm.Day = day;
  tm.Hour = hour;
  tm.Minute = minute;
  tm.Second = second;

  time_t t = makeTime(tm);

  return (long)t;
}

void repeatString(const char *str, int times, char *result) // Function to repeat a string (inspired from JS repeat function)
{
  result[0] = '\0'; // Set the result to an empty but not null string

  for (int i = 0; i < times; ++i) // Loop through the times
  {
    strcat(result, str); // Concatenate the string to the result
  }
}

void setup()
{

  clock.begin();      // Begin the clock
  Serial.begin(9600); // Begin the serial monitor
  while (!Serial)     // Wait for the serial monitor to open
  {
    ;
  }

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Begin the display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Arduino Arcade Time");
  display.drawBitmap(0, 7, getIcon("stopwatch"), 32, 23, WHITE); // Draw the stopwatch icon
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

  String fv = WiFi.firmwareVersion(); // Get the firmware version
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  { // Check if the firmware version is less than the latest version
    Serial.println("Please upgrade the firmware");
    display.clearDisplay();
    display.drawBitmap(0, 10, getIcon("error"), 32, 23, WHITE);
    display.setCursor(35, 10);
    display.print("Please upgrade");
    display.setCursor(35, 20);
    display.print("the WIFI firmware");

    display.display();
    delay(5000);
  }

  int frame = 0;                 // Frame counter
  const char *dot = ".";         // Dot string
  while (status != WL_CONNECTED) // Check if the WiFi is not connected
  {

    char result[100];
    repeatString(dot, frame + 1, result); // Repeat the dot string

    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    display.clearDisplay();
    display.setCursor(35, 10);
    display.print("Connecting" + String(result));
    display.setCursor(35, 20);
    display.print(ssid);
    if (frame == 0)
    {
      display.drawBitmap(0, 10, getIcon("hourglass_top"), 32, 23, WHITE);
    }
    else if (frame == 1)
    {
      display.drawBitmap(0, 10, getIcon("hourglass_split"), 32, 23, WHITE);
    }
    else if (frame == 2)
    {
      display.drawBitmap(0, 10, getIcon("hourglass_bottom"), 32, 23, WHITE);
    }
    display.display();
    frame++;
    if (frame > 2) // If frame is greater than 2 reset it to 0
    {
      frame = 0;
    }
    status = WiFi.begin(ssid, pass); // Begin the WiFi connection
  }

  printWifiStatus(); // Print the WiFi status
  delay(5000);
  Serial.println("\nStarting connection to server...");
  timeClient.begin();                        // Begin the time client
  timeClient.update();                       // Update the time client
  auto unixTime = timeClient.getEpochTime(); // Get the Unix time

  clock.setDateTime(unixTime); // Set the clock to the Unix time

  RTCDateTime currentTime = clock.getDateTime(); // Get the cur rent time
  Serial.print("Unix time = ");
  Serial.print(unixTime);
  Serial.print(" | ");
  Serial.println(currentTime.unixtime);

  Serial.println("The clock was just set to: ");
  Serial.print(currentTime.year);
  Serial.print("-");
  Serial.print(currentTime.month);
  Serial.print("-");
  Serial.print(currentTime.day);
  Serial.print(" ");
  Serial.print(currentTime.hour);
  Serial.print(":");
  Serial.print(currentTime.minute);
  Serial.print(":");
  Serial.print(currentTime.second);
  Serial.println("");

  http_request();
}

void loop()
{


}

void read_response() // Function to read the response
{
  if (client.available()) // Check if the client is available (has data to read)
  {
    Serial.println("reading response");
    String line = client.readStringUntil('\r'); // Read the response until a carriage return

    Serial.print(line);
    if (line == "\n") // Check if the line is a new line
    {
      Serial.println("headers received");
      String payload = client.readString(); // Read the payload
      jsonResult = JSON.parse(payload);     // Parse the payload
      Serial.println(jsonResult);
    }
  }
}

void http_request() // Function to make the HTTP request
{

  client.stop(); // Stop the client (if it is already connected)

  Serial.println("\nStarting connection to server...");
  if (client.connect(host, 80)) // Connect to the host
  {
    Serial.println("connected to server");
    String request = "/api/session/" + String(slackid); // request path
    client.println("GET " + request + " HTTP/1.1");     // Send the GET request
    client.println("Host: " + String(host));
    client.println("Authorization: Bearer " + String(apiKey));
    client.println("Connection: close");
    client.println();
    lastConnectionTime = millis(); // Set the last connection time to the current time
  }
  else
  { // If the connection failed
    Serial.println("connection failed");
  }
}

void printWifiBar() // Function to print the WiFi bar
{
  long rssi = WiFi.RSSI(); // Get the RSSI (Received Signal Strength Indicator)

  // It is a simple function that draws a bar based on the signal strength
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

void printWifiStatus() // Function to print the WiFi status
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
