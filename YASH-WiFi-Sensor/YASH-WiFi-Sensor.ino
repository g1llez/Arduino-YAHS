/* 
Author: Gilles Auclair
Date: 2024-10-29

This WiFi modules reads the temperature and humidity
and provide the info on a webserver with the proper parameters

*/


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTTYPE DHT11

// WiFi parameters
const char* ssid = "BenGi"; 
const char* password = "Aucl4ir!"; 

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
#define Expiration 300

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
  delay(1000);

  // Connecting to WiFi
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(5000);
  }

  Serial.println("WiFi Connected");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());

  // Starting the web server
  Serial.println("Starting WebServer");  
  server.on("/get_temp/", HTTP_GET, handleSentVar); // when the server receives a request with /data/ in the string then run the handleSentVar function
  server.begin();

  // Starting NTP sync
  timeClient.begin();

}

void loop() {

  // Updating the time
  timeClient.update();

  // Reading the temperature
  bool DataExpired = ( ( timeClient.getEpochTime() - lastUpdate ) > Expiration || temperature == -99 );
  bool DataRead = false;

  if ( DataExpired ) {
    Serial.println("Data expired, updating");
    delay(2000);
    temperature = dht.readTemperature();
    if (isnan(temperature)) {
 
      Serial.println("Error reading temperature");
      temperature = -99;
 
      digitalWrite(LED_BUILTIN,HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN,LOW);
      delay(100);
      digitalWrite(LED_BUILTIN,HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN,LOW);

    } else {
      Serial.println("Temperature : " + String(temperature));
      DataRead = true;
      lastUpdate = timeClient.getEpochTime();
    }

  }

  if ( !DataRead && DataExpired ) { temperature = -99; }
  
  // Check if there's a web client ready
  server.handleClient();
  delay(2000);

}

void handleSentVar() {
  server.send(200, "text/html", String(temperature) );  // return to sender
}
