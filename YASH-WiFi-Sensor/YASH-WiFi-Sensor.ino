/* 
Author: Gilles Auclair
Date: 2024-11-12

This WiFi modules reads the temperature and humidity
and provide the info on a webserver with the proper parameters

*/

#include "params.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
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
#define OID_HUMIDITY ".2"

// Temperature and humidity sensor
DHT dht(4, DHTTYPE); // GPIO4 = D2

// NTP and Timezone config
#define MY_NTP_SERVER "ca.pool.ntp.org"           
#define MY_TZ "EST+5EDT,M3.2.0/2,M11.1.0/2"   
time_t timeNow;
time_t timeLastUpdate;
tm tmLastUpdate;

// Set expiration on data read in seconds
#define Expiration 5

char debug[50] = {0};

// Date/time of last update
//long lastUpdate = -1;
bool DataExpired = true;

// Temperature and humidity
const char* DATA_UNKNOWN = "99.99";
char strTemperature[6];
char strHumidity[6];

// Function for SNMP to be able to get the last Temp
const std::string getTemperature() {
    return strTemperature;
}

// Function for SNMP to be able to get the last Temp
const std::string getHumidity() {
    return strHumidity;
}

// Fonction pour construire le OID complet
const char* getFullOID(const char* subOID) {
    static char fullOID[50];
    snprintf(fullOID, sizeof(fullOID), "%s%s", OID_YASH, subOID);
    return fullOID;
}

void setup(void)
{ 

  dht.begin();

  // Prepare the builtin led for debug
  pinMode(LED_BUILTIN, OUTPUT);

  // Initial state, LED is off
  digitalWrite(LED_BUILTIN, LOW);

  // Preparing the serial debug console
  Serial.begin(9600);

  // Initial state of data unknown
  strcpy(strTemperature, DATA_UNKNOWN);
  strcpy(strHumidity, DATA_UNKNOWN);

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
  server.on("/debug/", HTTP_GET, handleHTTPDebug);
  server.on("/api/status", HTTP_GET, handleHTTPJSON);
  
  server.begin();

  // Set timezone
  configTime(MY_TZ, MY_NTP_SERVER);
   
  // Setting up SNMP
  snmp.setUDP(&snmpUDP);  

  // Add temperature and humidity to OID
  const char* temperatureOID = getFullOID(OID_TEMP);
  snmp.addDynamicReadOnlyStringHandler(temperatureOID, getTemperature);
  const char* humidityOID = getFullOID(OID_HUMIDITY);  
  snmp.addDynamicReadOnlyStringHandler(humidityOID, getHumidity);
  
  // Starting SNMP
  snmp.begin();

  // Setting Arduino OTA
  ArduinoOTA.begin();

  delay(1000);

}

void loop() {


  // Updating the time
  time(&timeNow);

  // Checking if the data are expired or unknown
  DataExpired = ( ( timeNow - timeLastUpdate ) > Expiration || strTemperature == DATA_UNKNOWN || strHumidity == DATA_UNKNOWN );
  bool DataTempRead = false;
  bool DataHumRead = false;
  bool DataRead = false;
  float temp;
  float hum;

  // Updating if required
  if ( DataExpired ) {

    Serial.println("Data expired, updating");
    temp = dht.readTemperature() + DHT11_OFFSET;
    hum = dht.readHumidity();

    if (isnan(temp)) {
 
      Serial.println("Error reading temperature");
      strcpy(strTemperature, DATA_UNKNOWN);

    } else {
      
      sprintf(strTemperature, "%.2f", temp);
      Serial.printf("Temperature : ", strTemperature);

      DataTempRead = true;

    }

    if (isnan(hum)) {
 
      Serial.println("Error reading humidity");
      strcpy(strHumidity, DATA_UNKNOWN);

    } else {
      
      sprintf(strHumidity, "%.2f", hum);
      //dtostrf(hum, 4, 2, debug);
      Serial.printf("Humidity : ", strHumidity);

      DataHumRead = true;

    }

  }

  DataRead = DataTempRead && DataHumRead;
  
  if ( DataRead ) { time(&timeNow); time(&timeLastUpdate); localtime_r(&timeNow, &tmLastUpdate); }  
  if ( !DataTempRead && DataExpired ) { strcpy(strTemperature, DATA_UNKNOWN); }
  if ( !DataHumRead && DataExpired ) { strcpy(strHumidity, DATA_UNKNOWN); }
  
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



void handleHTTPDebug() {
  Serial.println(server.client().remoteIP());
  server.send(200, "text/html", debug);  // return to sender
}

void handleHTTPJSON() {

  Serial.println(server.client().remoteIP());

  char strDate[21];
  char strFanRunning[4];
  char strDataExpired[4];

  strcpy(strDataExpired, DataExpired ? "Oui" : "Non");

  sprintf(strDate, "%4d-%02d-%02d %02d:%02d:%02d", tmLastUpdate.tm_year + 1900, tmLastUpdate.tm_mon + 1, tmLastUpdate.tm_mday, tmLastUpdate.tm_hour, tmLastUpdate.tm_min, tmLastUpdate.tm_sec);

  String response = "";
  response += "{\n";
  response += "  \"lastTemp\": " + String(strTemperature) + ",\n";  
  response += "  \"lastHum\": " + String(strHumidity) + ",\n";  
  response += "  \"lastReading\": \"" + String(strDate) + "\",\n";
  response += "  \"dataExpired\": \"" + String(strDataExpired) + "\",\n";
  response += "  \"expirationDelay\": " + String(Expiration) + "\n";
  response += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type"); 
  server.send(200, "application/json", response);

}
