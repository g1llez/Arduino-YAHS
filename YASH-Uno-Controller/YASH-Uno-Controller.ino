/* 
Author: Gilles Auclair
Date: 2024-05-15

This modules will activate a relay if the temperature is higher than the trigger

*/

#include <TimeLib.h>

// Temperature and humidity sensor
#include <DHT11.h> 
DHT11 dht11(2); // Digital 2

int temperature = -99;
int humidity = -99;

// Set expiration on data in seconds
#define Expiration 500

// Keep track of last update
time_t lastUpdate = -1;

void setup() {

  // Preparing the serial debug console
  Serial.begin(9600);

  setTime(15,1,0,13,11,1979);

}

void loop() {
  
  // Reading the temperature
  bool DataExpired = ( now() - lastUpdate ) > Expiration;
  bool DataRead = false;

  if ( DataExpired ) {

    Serial.println("Updating...");
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

    } else {
      
      DataRead = true;
      lastUpdate = now();

    }  

  }

  if ( !DataRead && DataExpired ) { temperature = -99; }

  Serial.println("Temperature : " + String(temperature));

  delay(1000);
}
