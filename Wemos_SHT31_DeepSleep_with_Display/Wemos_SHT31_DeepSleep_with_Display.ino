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
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

Adafruit_SHT31 sht31 = Adafruit_SHT31();

const char* ssid = "";
const char* password = "";

const char* emon_url ="";
const int node = 1;
const char* apikey="";

int WLAN_Timeout=10;

#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup() {

  Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  // init done

  // Clear the buffer.
  display.clearDisplay();

   // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);

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

  display.print("T: ");
  display.print(t);
  display.print((char)247);
  display.println("C");
  display.print("RH: ");
  display.print(h);
  display.println("%");

  display.print("V: ");
  display.print((float)analogRead(A0)/1024*4.5);
  display.println("V");
  display.println("Connecting");
  display.display();

  WiFi.begin(ssid, password);

  Serial.print("Connecting");
 
  while (WiFi.status() != WL_CONNECTED && WLAN_Timeout>0) {
 
    delay(1000);
    Serial.print(".");
    display.print(".");
    display.display();
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
  else
  {
    display.print("Failed");
    display.display();

  }
    
  Serial.println("End of Program go to sleep 60s");
  ESP.deepSleep(60e6); // 20e6 is 10 seconds
}


void loop() {

}
