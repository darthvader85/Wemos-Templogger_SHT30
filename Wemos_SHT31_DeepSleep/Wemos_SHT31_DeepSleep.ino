/*************************************************** 
  This is an example for the SHT31-D Humidity & Temp Sensor

  Designed specifically to work with the SHT31-D sensor from Adafruit
  ----> https://www.adafruit.com/products/2857

  These sensors use I2C to communicate, 2 pins are required to  
  interface
 ****************************************************/
 
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();

const char* ssid = "";
const char* password = "";

const char* emon_url ="";
const int node = 1;
const char* apikey="";

int WLAN_Timeout=10;

void setup() {

  Serial.begin(9600);

  while (!Serial)
  delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("SHT31 test");
  if (! sht31.begin(0x45)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  
  String url_str;
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  Serial.println("SHT31");
  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp °C = "); Serial.println(t);
  } else { 
    Serial.println("Failed to read temperature");
  }

  WiFi.begin(ssid, password);

  Serial.print("Connecting");
 
  while (WiFi.status() != WL_CONNECTED && WLAN_Timeout>0) {
 
    delay(1000);
    Serial.print(".");
    WLAN_Timeout--;
  }
  Serial.println();

  delay(100);

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
 
    HTTPClient http;  //Declare an object of class HTTPClient

    url_str = emon_url;
    url_str += "/input/post.json?node=";
    url_str += node;
    url_str += "&json={Temp:";
    url_str += t;
    url_str += ",Hum:";
    url_str += h;
    url_str += "}&apikey=";
    url_str += apikey;

    Serial.println(url_str);
 
    http.begin(url_str);  //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
 
    if (httpCode > 0) { //Check the returning code
 
      String payload = http.getString();   //Get the request response payload
      Serial.println(payload);                     //Print the response payload
 
    }
 
    http.end();   //Close connection

  }
    
  Serial.println("End of Program go to sleep 60s");
  ESP.deepSleep(60e6); // 20e6 is 10 seconds
}


void loop() {

}
