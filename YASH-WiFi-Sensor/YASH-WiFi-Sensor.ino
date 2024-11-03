/* 
Author: Gilles Auclair
Date: 2024-11-03

This WiFi modules reads the temperature and humidity
and provide the info on a webserver with the proper parameters

*/

#include "params.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoOTA.h>

//const char* ssid = "MySSID"; 
//const char* password = "MyPass"; 

#define DHTTYPE DHT11
#define DHT11_OFFSET -7.6;

// WiFi Server
ESP8266WebServer server(80);

// Temperature and humidity sensor
DHT dht(4, DHTTYPE); // GPIO4 = D2

// NTP Config
WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// Set expiration on data read in seconds
#define Expiration 5

long lastUpdate = -1;
float temperature = -99;
float humidity = -99;

void setup(void)
{ 

  dht.begin();

  // Prepare the builtin led for debug
  pinMode(LED_BUILTIN, OUTPUT);

  // Initial state, LED is off
  digitalWrite(LED_BUILTIN, LOW);

  // Preparing the serial debug console
  Serial.begin(9600);

  // Connecting to WiFi
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("WiFi Connected");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
  
  // Setting auto-reconnect
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Starting the web server
  Serial.println("Starting WebServer"); 
  server.on("/get_temp/", HTTP_GET, handleHTTPGetTemp);
  server.begin();

  // Starting NTP sync
  timeClient.begin();

  // Setting Arduino OTA
  ArduinoOTA.begin();

  delay(1000);

}

void loop() {

  // Updating the time
  timeClient.update();

  // Reading the temperature
  bool DataExpired = ( ( timeClient.getEpochTime() - lastUpdate ) > Expiration || temperature == -99 );
  bool DataRead = false;

  if ( DataExpired ) {

    Serial.println("Data expired, updating");
    temperature = dht.readTemperature() + DHT11_OFFSET;

    if (isnan(temperature)) {
 
      Serial.println("Error reading temperature");
      temperature = -99;

    } else {

      Serial.println("Temperature : " + String(temperature));
      DataRead = true;
      lastUpdate = timeClient.getEpochTime();

    }

  }

  if ( !DataRead && DataExpired ) { temperature = -99; }
  
  // Check if there's a web client ready
  server.handleClient();

  // Handling Arduino OTA
  ArduinoOTA.handle();

  delay(1000);

}

void handleHTTPGetTemp() {
  Serial.println(server.client().remoteIP());
  server.send(200, "text/html", String(temperature) );  // return to sender
}
