#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define RELAY_GPIO 4 // GPIO4 = D2

// WiFi parameters to be configured
const char* ssid = "BenGi"; // Write here your router's username
const char* password = "Aucl4ir!"; // Write here your router's passward

String srvTemperatureSensor = "192.168.13.59";
String srvURL = "/get_temp/";
#define MAX_TEMP 21

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

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("WiFi Connected");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());

  // Starting NTP time sync
  timeClient.begin();

  delay(5000);
}

void loop() {

    int temperature = getTemperature();

    updateFan(temperature, MAX_TEMP);

    delay(10000);

}

void updateFan(int CurrentTemp, int TriggerTemp) {

  if ( CurrentTemp == -99 ) {
    // Temperature unknown, starting fan
    Serial.println("[FAN] Starting - Temperature unknown");
    digitalWrite(RELAY_GPIO, HIGH);
  }
  else if ( CurrentTemp > TriggerTemp ) {
    // Temperature higher than trigger, starting fan
    Serial.println("[FAN] Starting - Temperature : " + String(CurrentTemp));
    digitalWrite(RELAY_GPIO, HIGH);
  } else {
    // Temperature lower than trigger, stopping fan
    Serial.println("[FAN] Stopping - Temperature : " + String(CurrentTemp));
    digitalWrite(RELAY_GPIO, LOW);
  }

}

int getTemperature() {

  // Starting HTTP Client
  WiFiClient client;
  HTTPClient http;  
  int temperature = -99;

  if (http.begin(client, "http://" + srvTemperatureSensor + srvURL )) {

    // start connection and send HTTP header
    int httpCode = http.GET();    

    // httpCode will be negative on error
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        if ( isNumber(payload) ) {          
            temperature = payload.toInt();
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
  
  size_t start = 0;
  if (str.charAt(start) == '-') { start = 1; }

  // Iterate through each character of the string
  for (size_t i = start; i < str.length(); i++) {
    // Check if the current character is not a digit
    if (!isDigit(str.charAt(i))) {
      return false;
    }
  }

  return true; // All characters are digits

}
