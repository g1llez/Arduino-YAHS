/* 
Author: Gilles Auclair
Date: 2024-11-03

This WiFi modules reads the temperature over the network
and if it's unknown or above the threshold, the relay will
activate

*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Arduino_SNMP_Manager.h>
#include "params.h"

//const char* ssid = "MySSID"; 
//const char* password = "MyPass"; 

// SNMP
const int snmpVersion = 1; // SNMP Version 1 = 0, SNMP Version 2 = 1

// IPAddress myIP(192, 168, 0, 10);
// IPAddress remoteIP(192, 168, 0, 11);
const char* community = "public";
const char* OID_YASH = ".1.3.6.1.4.1.1313.1.1";
#define OID_TEMP ".1"
WiFiUDP snmpUDP;

// Temperature var for snmp
ValueCallback *callbackTemperature;

char* ifTemperature = new char[6];
char fullOID[50];
SNMPManager snmp = SNMPManager(community);             // Starts an SNMPManager to listen to replies to get-requests
SNMPGet snmpRequest = SNMPGet(community, snmpVersion); // Starts an SNMPGet instance to send requests

// Fonction pour construire le OID complet
const char* getFullOID(const char* subOID) {
    snprintf(fullOID, sizeof(fullOID), "%s%s", OID_YASH, subOID);
    return fullOID;
}

#define RELAY_GPIO 4 // GPIO4 = D2

// WiFi Server
ESP8266WebServer server(80);

#define MAX_TEMP 21.5            // Température minimum désiré
#define STATE_UPDATE_DELAY 300   // Délais minimum avant un nouveau changement d'état

// NTP Config
WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// Last temperature reading
float LastTemp = 99.99;
// Date/time of the last change
long lastChange = -1;

char debug[100] = {0};

void setup(void)
{ 

  // Setting relay
  pinMode(RELAY_GPIO, OUTPUT);

  // Starting serial port
  Serial.begin(9600);

  strcpy(ifTemperature, "99.99");
  
  Serial.println("[NET] Connecting to WiFi");

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("[NET] WiFi Connected");
  Serial.print("[NET] IP Address : ");
  Serial.println(WiFi.localIP());

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Starting NTP time sync
  Serial.println("[NTP] Starting NTP");
  timeClient.begin();

  // Starting the web server
  Serial.println("Starting WebServer"); 
  server.on("/debug/", HTTP_GET, handleHTTPDebug);
  server.begin();

  // Starting SNMP
  snmp.setUDP(&snmpUDP); 
  snmp.begin();      

  // Add handler to get temperature via snmp
  const char* temperatureOID = getFullOID(OID_TEMP);
  callbackTemperature = snmp.addStringHandler(remoteIP, temperatureOID, &ifTemperature);

  // Starting OTA support
  Serial.println("[OTA] Starting OTA support");
  ArduinoOTA.begin();

  delay(1000);

}

void loop() {

    // Updating the time with NTP
    timeClient.update();

    bool DataExpired = ( ( timeClient.getEpochTime() - lastChange ) > STATE_UPDATE_DELAY || LastTemp == 99.99 );

    if (DataExpired) {
       
      Serial.println("[MAIN] State change delay expired, getting temperature and updating fan status if required");

      // Get the last values from sensor via snmp
      getSNMP();

      // Updating the fan state
      LastTemp = atof(ifTemperature);
      updateFan(LastTemp, MAX_TEMP);

      lastChange = timeClient.getEpochTime();

      //snprintf(debug, sizeof(debug), "Temp %s : Last changed: %s", ifTemperature, lastChange);

    }

    // Check if there's a web client ready
    server.handleClient();

    // Handle snmp
    snmp.loop();

    // Handle OTA
    ArduinoOTA.handle();

    delay(1000);

}

void getSNMP()
{

  const char* temperatureOID = getFullOID(OID_TEMP);
  
  // Get temperature
  snmpRequest.addOIDPointer(callbackTemperature);  
  snmpRequest.setIP(myIP);
  snmpRequest.setUDP(&snmpUDP);
  snmpRequest.setRequestID(rand() % 5555);
  snmpRequest.sendTo(remoteIP);
  snmpRequest.clearOIDList();

}

void updateFan(float CurrentTemp, float TriggerTemp) {

  uint8_t DesireValue = HIGH;
  String status ="";

  if ( CurrentTemp == 99.99 ) {
    // Temperature unknown, starting fan
    status = "[FAN] Temperature unknown";
    DesireValue = HIGH;
  }
  else if ( CurrentTemp < TriggerTemp ) {
    // Temperature lower than trigger, starting fan
    status = "[FAN] Temperature : " + String(CurrentTemp) + " Trigger : " + String(TriggerTemp);
    DesireValue = HIGH;
  } else {
    // Temperature lower than trigger, stopping fan
    status = "[FAN] Temperature : " + String(CurrentTemp) + " Trigger : " + String(TriggerTemp);
    DesireValue = LOW;
  }

  status = status + " State : ";

  if ( digitalRead(RELAY_GPIO) != DesireValue ) {
    status += DesireValue ? "Starting" : "Stopping";
    digitalWrite(RELAY_GPIO, DesireValue);
  } else {
    status += DesireValue ? "Running" : "Stopped";
  }
  
  Serial.println(status);

}

bool isNumber(const String &str) {

  // Check if the string is empty
  if (str.length() == 0) {
    return false;
  }

  bool DecimalFound = false;
  
  size_t start = 0;
  if (str.charAt(start) == '-') { start = 1; }

  // Iterate through each character of the string
  for (size_t i = start; i < str.length(); i++) {
    // Check if the current character is not a digit
    if (!isDigit(str.charAt(i))) {

      // Check if it's the first comma, else it's not a number 
      if (str.charAt(i) == '.') {
        if (DecimalFound) {
          return false;
        } else {
           DecimalFound = true;
        }
      } 
      else {
        return false;
      }

    }
  }

  return true; // All characters are digits

}


void handleHTTPDebug() {
  Serial.println(server.client().remoteIP());
  server.send(200, "text/html", debug);  // return to sender
}
