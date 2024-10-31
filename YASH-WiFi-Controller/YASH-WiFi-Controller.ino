#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define RELAY_GPIO 4 // GPIO4 = D2

// WiFi parameters to be configured
const char* ssid = "BenGi"; 
const char* password = "Aucl4ir!"; 

String srvTemperatureSensor = "192.168.13.59";
String srvURL = "/get_temp/";

#define MAX_TEMP 21.0           // Température minimum désiré
#define STATE_UPDATE_DELAY 60   // Délais minimum avant un nouveau changement d'état

long lastChange = -1; // Date/Heure du dernier changement

// Number of milliseconds to wait without receiving any data before we give up
const int NetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int NetworkDelay = 1000;

// NTP Config
WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

void setup(void)
{ 

  // Setting relay
  pinMode(RELAY_GPIO, OUTPUT);

  // Starting serial port
  Serial.begin(9600);

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

  delay(1000);

  Serial.println("[OTA] Starting OTA support");
  ArduinoOTA.begin();

}

void loop() {

    // Updating the time with NTP
    timeClient.update();

    bool DataExpired = ( ( timeClient.getEpochTime() - lastChange ) > STATE_UPDATE_DELAY );

    if (DataExpired) {
       
      Serial.println("[MAIN] State change delay expired, getting temperature and updating fan status if required");

      float temperature = getTemperature();

      updateFan(temperature, MAX_TEMP);

      lastChange = timeClient.getEpochTime();

    }

    ArduinoOTA.handle();

    delay(1000);

}

void updateFan(float CurrentTemp, float TriggerTemp) {

  uint8_t DesireValue = HIGH;
  //char status[] = {""};
  String status ="";

  if ( CurrentTemp == -99 ) {
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

float getTemperature() {

  // Starting HTTP Client
  WiFiClient client;
  HTTPClient http;  
  float temperature = -99;
  
  http.addHeader("Accept", "*/");
  http.addHeader("User-Agent", "ESP8266");
  http.setTimeout(5000);

  if (http.begin(client, "http://" + srvTemperatureSensor + srvURL )) {

    // start connection and send HTTP header
    int httpCode = http.GET();    
    
    // httpCode will be negative on error
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        if ( isNumber(payload) ) {          
            temperature = payload.toFloat();
        } else {
          Serial.println("[HTTP] Error while interpreting temperature. Data received : " + payload);
        }
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

  } else {
    Serial.println("[HTTP] Unable to connect");
  }

  return temperature;

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
