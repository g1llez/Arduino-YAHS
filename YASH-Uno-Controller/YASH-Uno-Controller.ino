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
#define ExpirationTemp 30 // Expiration for the temperature
#define ExpirationFan 500 // Expiration for the fan

// Relay
#define RELAY_PIN 3 // Digital 3

// Trigger temperature
#define MAX_TEMP 21

#include <LedControl.h>

/*
Pin 12 -> DataIn (DIN)
Pin 11 -> Load (CS)
Pin 10 -> Clock (CLK)
The last parameters is the number of display connected
*/
LedControl Matrix=LedControl(12,10,11,1);

#define DELAY_LONG 500

#define NB_ROW 8
#define DECALAGE_POS_1 1
#define DECALAGE_POS_2 5

// Keep track of last update
time_t lastTempUpdate = -1;
time_t lastFanUpdate = -1;

bool FanRunning = false;

void setup() {

  // Preparing the serial debug console
  Serial.begin(9600);
  delay(1000);

  // Setting relay
  pinMode(RELAY_PIN, OUTPUT);

  // Wake up call for the Matrix as it start in power-saving mode
  Matrix.shutdown(0, false); // Wake up
  Matrix.setIntensity(0, 8); // Intensity to medium
  Matrix.clearDisplay(0);    // Clear display

  // Initialize time
  setTime(0,0,0,1,1,1970);
  lastFanUpdate = now() - ( ExpirationFan + 1 );
  lastTempUpdate = now() - ( ExpirationTemp + 1 );
}

void loop() {
  
  // Reading the temperature
  bool DataExpired = ( ( now() - lastTempUpdate ) > ExpirationTemp || temperature == -99) ;
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
      lastTempUpdate = now();

    }  

  }

  if ( !DataRead && DataExpired ) { temperature = -99; }
  
  if ( ( ( now() - lastFanUpdate ) > ExpirationFan ) ) {
     FanRunning = updateFan(temperature, MAX_TEMP);
     lastFanUpdate = now();
  }

  UpdateMatrix(temperature, FanRunning);

}

bool updateFan(int CurrentTemp, int TriggerTemp) {

  bool Running = false;

  if ( CurrentTemp == -99 ) {
    // Temperature unknown, starting fan
    Serial.println("[FAN] Starting - Temperature unknown");
    digitalWrite(RELAY_PIN, HIGH);
    Running = true;
  }
  else if ( CurrentTemp > TriggerTemp ) {
    // Temperature higher than trigger, starting fan
    Serial.println("[FAN] Starting - Temperature : " + String(CurrentTemp));
    digitalWrite(RELAY_PIN, HIGH);
    Running = true;

  } else {
    // Temperature lower than trigger, stopping fan
    Serial.println("[FAN] Stopping - Temperature : " + String(CurrentTemp));
    digitalWrite(RELAY_PIN, LOW);
  }

  return Running;

}

void UpdateMatrix(int temperature, bool FanActive) {

  // Fan
  byte fan1[8]{B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,B00011100,B00001000};
  byte fan2[8]{B00000000,B00000000,B00000000,B00000000,B00000000,B00010100,B00001000,B00010100};
  
  // Digit
  byte number[10][8]{
    {B00000000,B11100000,B10100000,B11100000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B11000000,B01000000,B11100000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B11000000,B01000000,B01100000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B11100000,B01100000,B11100000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B10100000,B11100000,B00100000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B01100000,B01000000,B11000000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B10000000,B11100000,B11100000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B11100000,B00100000,B00100000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B01100000,B11100000,B11100000,B00000000,B00000000,B00000000,B00000000},
    {B00000000,B11100000,B11100000,B00100000,B00000000,B00000000,B00000000,B00000000}
  };

  byte Negatif[8]{B00000000,B00000000,B10000000,B00000000,B00000000,B00000000,B00000000,B00000000};

  byte Output = B00000000;

  byte TempPos1 = abs(temperature) / 10;
  byte TempPos2 = abs(temperature) % 10;
  bool Positif = temperature >= 0;

  // Display fan in position 1
  for (int x = 0; x < NB_ROW; x++) {
    Output = fan1[x];
    Output = Output | number[TempPos1][x] >> DECALAGE_POS_1;
    Output = Output | number[TempPos2][x] >> DECALAGE_POS_2;
    if ( !Positif ) { Output = Output | Negatif[x]; }
    Matrix.setRow(0,x,Output);
  }
  
  delay(DELAY_LONG);

  // Display fan in position 2 if fan active
  
  for (int x = 0; x < NB_ROW; x++) {
    if (FanActive) {
      Output = fan2[x];
    }
    else {
      Output = fan1[x];
    }
    Output = Output | number[TempPos1][x] >> DECALAGE_POS_1;
    Output = Output | number[TempPos2][x] >> DECALAGE_POS_2;
    if ( !Positif ) { Output = Output | Negatif[x]; } 
    Matrix.setRow(0,x,Output);
  }

  delay(DELAY_LONG);
  
}