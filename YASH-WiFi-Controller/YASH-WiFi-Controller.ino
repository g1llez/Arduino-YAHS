#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define RELAY_GPIO 4

// WiFi parameters to be configured
const char* ssid = "BenGi-TP2.4"; // Write here your router's username
const char* password = "Aucl4ir!"; // Write here your router's passward

String srvTemperatureSensor = "192.168.13.216";
String TempURL = "/get_temp/";
#define MAX_TEMP 20

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

    if ( temperature != -99 ) {
      updateFan(temperature, MAX_TEMP);
    }
    delay(10000);
}

void updateFan(int CurrentTemp, int TriggerTemp) {

  if ( CurrentTemp > TriggerTemp ) {
    digitalWrite(RELAY_GPIO, HIGH);
  } else {
    digitalWrite(RELAY_GPIO, LOW);
  }
}

bool getTemperature() {

  // Starting HTTP Client
  WiFiClient client;
  HTTPClient http;  

  if (http.begin(client, "http://192.168.13.216/get_temp/")) {

    // start connection and send HTTP header
    int httpCode = http.GET();
    int temperature = -99;

    // httpCode will be negative on error
    if (httpCode > 0) {
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println(payload);
        if ( isNumber(payload) ) {          
            temperature = payload.toInt();
        } else {
          Serial.println("Error while interpreting temperature. Data received : " + payload);
        }
        return temperature;
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

  } else {
    Serial.println("[HTTP] Unable to connect");
  }

}
bool isNumber(const String &str) {

  // Check if the string is empty
  if (str.length() == 0) {
    return false;
  }

  // Iterate through each character of the string
  for (size_t i = 0; i < str.length(); i++) {
    // Check if the current character is not a digit
    if (!isdigit(str.charAt(i))) {
      return false;
    }
  }

  return true; // All characters are digits

}
