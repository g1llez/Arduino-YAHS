/* 
Author: Gilles Auclair
Date: 2024-05-07

This WiFi modules reads the temperature and humidity
and provide the info on a webserver with the proper parameters

*/


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT11.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


// WiFi parameters
const char* ssid = "BenGi-TP2.4"; 
const char* password = "Aucl4ir!"; 

// WiFi Server
ESP8266WebServer server(80);

// Temperature and humidity sensor
DHT11 dht11(2); // GPIO2 = D4

#define MAX_TRIES 1
int temperature = -99;
int humidity = -99;

// NTP Config
WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// Set expiration on data read in seconds
#define Expiration 300

long lastUpdate = -1;

void setup(void)
{ 

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
    delay(1000);
  }

  Serial.println("WiFi Connected");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());

  // Starting the web server
  Serial.println("Starting WebServer");  
  server.on("/get_temp/", HTTP_GET, handleSentVar); // when the server receives a request with /data/ in the string then run the handleSentVar function
  server.begin();

  timeClient.begin();

}

void loop() {

  // Updating the time
  timeClient.update();

  delay(5000);


  // Reading the temperature
  int x = MAX_TRIES;
  bool ValueRead = false;
  bool DataExpired = ( timeClient.getEpochTime() - lastUpdate ) > Expiration;
  while ( (x >= 0) && !ValueRead && DataExpired ) {
    Serial.println("Data expired, updating");
    int result = dht11.readTemperatureHumidity(temperature, humidity);
    if ( result != 0 ) {
  
      Serial.println(DHT11::getErrorString(result));
      digitalWrite(LED_BUILTIN,HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN,LOW);
      delay(100);
      digitalWrite(LED_BUILTIN,HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN,LOW);
      x--;

    } else {
      ValueRead = true;
      lastUpdate = timeClient.getEpochTime();
    }
    delay(1000);

  }

  if ( !ValueRead && DataExpired ) { temperature = -99; }
  
  // Check if there's a web client ready
  server.handleClient();
  delay(1000);

}

void handleSentVar() {
  server.send(200, "text/html", String(temperature) );  // return to sender
}
