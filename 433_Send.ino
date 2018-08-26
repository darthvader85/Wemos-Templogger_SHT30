
#include <RFM69.h>  
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SHT31.h>

//*********************************************************************************************
// *********** IMPORTANT SETTINGS - YOU MUST CHANGE/ONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NETWORKID     2  // The same on all nodes that talk to each other
#define NODEID        2  // The unique identifier of this node
#define RECEIVER      1    // The recipient of packets

//Match frequency to the hardware version of the radio on your Feather
#define FREQUENCY     RF69_433MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HCW   true // set to 'true' if you are using an RFM69HCW module

#define SERIAL_BAUD   115200

//RFM69
#define RFM69_CS      0   // GPIO0/D3
#define RFM69_IRQ     15  // GPIO15/HCS/D8
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
#define RFM69_RST     2   // GPIO02/D4
#define PowerLevel    31
#define SendRetries   10
#define RetryWaitTime 100 //ms

#define SHT31_Adress 0x45

#define OLED_RESET -1 //--> No Reset Pin

//*********************************************************************************************


uint32_t packetnum = 0;  // packet counter, we increment per xmission
RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);

//SSD1306
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

//SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();


char buf[100]; 

void setup_oled()
{
  Serial.println("Init Oled...");
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  // init done

  // Clear the buffer.
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.display();

  Serial.println("Init Oled done...");
}

void setup_rfm69()
{
  
  // Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  delay(100);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  // Initialize radio
  if (!radio.initialize(FREQUENCY,NODEID,NETWORKID)) {
    Serial.println("radio initialize failed!");
  }
  if (IS_RFM69HCW) {
    radio.setHighPower(true);    // Only for RFM69HCW & HW!
    Serial.println("High Power Mode!");
  }

  radio.setPowerLevel(PowerLevel); // power output ranges from 0 (5dBm) to 31 (20dBm)

  radio.encrypt(ENCRYPTKEY);

  Serial.printf("\nTransmitting at %d MHz\r\n",FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.printf("Network: %d  Node: %d\r\n",NETWORKID,NETWORKID);
}

void setup_sht31()
{
  Serial.println("Init SHT31...");  
  //Init SHT31 with adress 0x45
  sht31.begin(SHT31_Adress);  
  
  Serial.println("Init SHT31 done...");
}

void setup() {

  //Turn WIFI OFF
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);

  //Init Serial Port
  Serial.begin(SERIAL_BAUD);
  Serial.printf("\r\nArduino RFM69HCW Transmitter\r\n");
  
  //Setup Display
  setup_oled();
  //Setup RFM69 433MHz
  setup_rfm69();
  //Setup SHT31
  setup_sht31();
}

void send_radio(float temp,float hum,float bat)
{

  uint32_t startMillis;
  static uint32_t deltaMillis = 0;

  //Prepare message to send...
  sprintf(buf,"%d,%3.2f,%3.2f,%3.2f",packetnum++,temp,hum,bat);
  //Debug Info
  Serial.printf("Sending: %s\r\n",buf);


  startMillis = millis();
  //Try to send data
  if (radio.sendWithRetry(RECEIVER, buf, strlen(buf)+1,SendRetries,RetryWaitTime)) 
  {
      deltaMillis = millis() - startMillis;
      Serial.printf("Send OK: %d ms\r\n",deltaMillis);

      display.printf("Send OK\r\n%dms",deltaMillis);
  }
  else
  {
      Serial.println("Send Fail");
      
      display.print("Send FAIL");
  }


  radio.receiveDone(); //put radio in RX mode

  display.display();
}

float get_sht31_temp()
{
  return sht31.readTemperature();
  float h = sht31.readHumidity();
}

float get_sht31_hum()
{
  return sht31.readHumidity();
}

float get_bat_voltage()
{
  return (float)analogRead(A0)/1024*4.5;
}


void loop() {
  float temp,hum,bat;

  temp=get_sht31_temp();
  hum=get_sht31_hum();  
  bat=get_bat_voltage(); 

  //Debug Values
  Serial.println();
  Serial.printf("Temp: %3.2f °C\r\n",temp);
  Serial.printf("Hum: %3.2f %\r\n",hum);
  Serial.printf("Bat: %3.2f V\r\n",bat);
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("TX 433MHz");
  display.printf("Tem:%3.1fC\r\n",temp);
  display.printf("Hum:%3.1f%%\r\n",hum);
  display.printf("Bat:%3.2fV\r\n",bat);
  display.display();
  
  send_radio(temp,hum,bat);
  
  delay(20000);  // Wait 1 second between transmits, could also 'sleep' here!


}
