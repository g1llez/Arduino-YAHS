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
#include <WiFiUdp.h>
#include <SNMP_Agent.h>

//const char* ssid = "MySSID"; 
//const char* password = "MyPass"; 

#define DHTTYPE DHT11
#define DHT11_OFFSET -7.6;

// WiFi Server
ESP8266WebServer server(80);

// SNMP Agent
SNMPAgent snmp("public", "private");
WiFiUDP snmpUDP;

const char* OID_YASH = ".1.3.6.1.4.1.1313.1.1";

#define OID_TEMP ".1"

// Temperature and humidity sensor
DHT dht(4, DHTTYPE); // GPIO4 = D2

// NTP Config
WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// Set expiration on data read in seconds
#define Expiration 5

char debug[50] = {0};

long lastUpdate = -1;
const char* TEMP_UNKNOWN = "99.99";
char strTemperature[6];

// Function for SNMP to be able to get the last Temp
const std::string getTemperature() {
    return strTemperature;
}

char fullOID[50];

// Fonction pour construire le OID complet
const char* getFullOID(const char* subOID) {
    snprintf(fullOID, sizeof(fullOID), "%s%s", OID_YASH, subOID);
    return fullOID;
}


//uint32_t humidity = 999;

void setup(void)
{ 

  dht.begin();

  // Prepare the builtin led for debug
  pinMode(LED_BUILTIN, OUTPUT);

  // Initial state, LED is off
  digitalWrite(LED_BUILTIN, LOW);

  // Preparing the serial debug console
  Serial.begin(9600);

  strcpy(strTemperature, TEMP_UNKNOWN);

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
  //server.on("/debug/", HTTP_GET, handleHTTPDebug);
  server.begin();

  // Starting NTP sync
  timeClient.begin();

   
  // Setting up SNMP
  snmp.setUDP(&snmpUDP);  

  // Add temperature to OID
  const char* temperatureOID = getFullOID(OID_TEMP);
  strcpy(debug, temperatureOID);

  snmp.addDynamicReadOnlyStringHandler(temperatureOID, getTemperature);

  // Starting SNMP
  snmp.begin();

  // Setting Arduino OTA
  ArduinoOTA.begin();

  delay(1000);

}

void loop() {

  // Updating the time
  timeClient.update();

  // Reading the temperature
  bool DataExpired = ( ( timeClient.getEpochTime() - lastUpdate ) > Expiration || strTemperature == TEMP_UNKNOWN );
  bool DataRead = false;
  float temp;

  if ( DataExpired ) {

    Serial.println("Data expired, updating");
    temp = dht.readTemperature() + DHT11_OFFSET;

    if (isnan(temp)) {
 
      Serial.println("Error reading temperature");
      strcpy(strTemperature, TEMP_UNKNOWN);

    } else {
      
      sprintf(strTemperature, "%.2f", temp);
      Serial.printf("Temperature : ", strTemperature);

      DataRead = true;
      lastUpdate = timeClient.getEpochTime();

    }

  }

  if ( !DataRead && DataExpired ) { strcpy(strTemperature, TEMP_UNKNOWN); }
  
  // Check if there's a web client ready
  server.handleClient();

  // Handle SNMP
  snmp.loop();

  // Handling Arduino OTA
  ArduinoOTA.handle();

  delay(1000);

}

void handleHTTPGetTemp() {
  Serial.println(server.client().remoteIP());
  server.send(200, "text/html", strTemperature);  // return to sender
}

/*

void handleHTTPDebug() {
  Serial.println(server.client().remoteIP());
  server.send(200, "text/html", debug);  // return to sender
}

*/