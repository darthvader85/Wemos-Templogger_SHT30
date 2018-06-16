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
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define OLED_RESET 4
Adafruit_SSD1306 display(LED_BUILTIN);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_BMP085 bmp;
Adafruit_BMP280 bme;

const char* ssid = "Hier gibts keinen";
const char* password = "";

WiFiServer server(80);

void setup() {
 
  Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);


   
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
 
    delay(1000);
    Serial.print("Connecting..");
    // Clear the buffer.
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("Connecting to ");
    display.println(ssid);
    display.display();
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Connected!");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();  
  
  delay(10000);
  
  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  if (!bmp.begin()) {
  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  }

  if (!bme.begin(0x76)) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // Start the server
  server.begin();
}


void loop() {

  String url_str;
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  float p_bmp180 = bmp.readPressure();
  float t_bmp180 = bmp.readTemperature();
  float a_bmp180 = bmp.readAltitude();
  float p_bmp280 = bme.readPressure();
  float t_bmp280 = bme.readTemperature();
  float a_bmp280 = bme.readAltitude();

  Serial.println("SHT31");
  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp °C = "); Serial.println(t);
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
  } else { 
    Serial.println("Failed to read humidity");
  }

  Serial.println("BMP180");
  if (! isnan(p_bmp180)) {  // check if 'is not a number'
    Serial.print("Pres. % = "); Serial.println(p_bmp180);
  } else { 
    Serial.println("Failed to read pressure");
  }


  Serial.println("BMP280");
  
  Serial.print("Pressure = ");
  Serial.print(p_bmp280);
  Serial.println(" Pa");
  Serial.println();

  // Clear the buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("T= ");
  display.print(t);
  display.print("C ,H= ");
  display.print(h);
  display.println("%");
  display.println("BMP180:");
  display.print("p=");
  display.print(p_bmp180);
  display.print(" t=");
  display.println(t_bmp180);
  display.print(" a=");
  display.println(a_bmp180);
  display.println("BMP280:");
  display.print("p=");
  display.print(p_bmp280);
  display.print(" t=");
  display.println(t_bmp280);
  display.print(" a=");
  display.println(a_bmp280);
  display.display();

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
 
    HTTPClient http;  //Declare an object of class HTTPClient

    url_str = "http://emon.sklammer.at/input/post.json?node=33&json={Temp:";
    url_str += t;
    url_str += ",Hum:";
    url_str += h;
    url_str += ",Pressure:";
    url_str += p_bmp180;
    url_str += ",Pressure_BMP280:";
    url_str += p_bmp280;
    url_str += "}&apikey=";
 
    http.begin(url_str);  //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
 
    if (httpCode > 0) { //Check the returning code
 
      String payload = http.getString();   //Get the request response payload
      Serial.println(payload);                     //Print the response payload
 
    }
 
    http.end();   //Close connection

  }
    
  WiFiClient client = server.available();
  
  if (!client)
  {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); //  do not forget this one
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("Test");
    client.println("</html>");

  }
    
  delay(1000);
}
