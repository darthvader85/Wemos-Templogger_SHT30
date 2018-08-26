
#include <RFM69.h> 
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SHT31.h>

//*********************************************************************************************
// *********** IMPORTANT SETTINGS - YOU MUST CHANGE/ONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NETWORKID     2  // The same on all nodes that talk to each other
#define NODEID        1  // The unique identifier of this node

//Match frequency to the hardware version of the radio on your Feather
#define FREQUENCY     RF69_433MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HCW   true // set to 'true' if you are using an RFM69HCW module

#define SERIAL_BAUD   115200

#define WLAN_Timeout_s 10;

//RFM69
#define RFM69_CS      0   // GPIO0/D3
#define RFM69_IRQ     15  // GPIO15/HCS/D8
#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ)
#define RFM69_RST     2   // GPIO02/D4
#define PowerLevel    31

//SSD1306
#define OLED_RESET -1 //--> No Reset Pin
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

//SHT31
#define SHT31_Adress 0x45

#define MeasureInterval_ms 20000
#define httpRetries 3


//WIFI
const char* ssid = "SSID";
const char* password = "PWD";
//Emon-Constants
const char* emon_url ="url";
const char* apikey="apikey";

//*********************************************************************************************



RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);

//SSD1306
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

//SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();

char buf[255];
uint32_t startMillis; //Time Buf for measurement

//Receiver Data
uint8_t senderId;
int16_t rssi;
char data[RF69_MAX_DATA_LEN];


void setup_wifi()
{
  uint8_t timeout = WLAN_Timeout_s;
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED && timeout>0) 
  { 
    Serial.print(".");
    timeout--;
    delay(1000);
  }

  if(WiFi.status() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip); 
  }
  else
  {
     Serial.println("Wifi connection failed!"); 
  }
}


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

  Serial.printf("\nReceiving at %d MHz\r\n",FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
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


  //Init Serial Port
  Serial.begin(SERIAL_BAUD);
  Serial.printf("\r\nArduino RFM69HCW Transmitter\r\n");
  //Setup WIFI
  setup_wifi();
  //Setup Display
  setup_oled();
  //Setup RFM69 433MHz
  setup_rfm69();
  //Setup SHT31
  setup_sht31();
}

bool receive_radio()
{
  bool bRetVal=false;
  
  //check if something was received (could be an interrupt from the radio)
  if (radio.receiveDone())
  {    

    //save packet because it may be overwritten
    senderId = radio.SENDERID;
    rssi = radio.RSSI;
    memcpy(data, (void *)radio.DATA, radio.DATALEN);
    
    //check if sender wanted an ACK
    if (radio.ACKRequested())
    {
      radio.sendACK();
    }
    radio.receiveDone(); //put radio in RX mode
    
    Serial.printf("*** RFM69 data received... *** Sender: %d RSSI: %d\r\n",senderId,rssi);
    Serial.printf("Data: %s\r\n",(const char *)data);

    
    
    bRetVal=true;
  }
  else
  {
    radio.receiveDone(); //put radio in RX mode
  }

  return bRetVal;
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

String getEmonString(uint8_t senderid,float temp,float hum,float bat,int8_t rssi)
{
  sprintf(buf,"%s/input/post.json?node=%d&json={Temp%d:%3.2f,Hum%d:%3.2f,Bat%d:%3.2f,RSSI%d:%d}&apikey=%s",emon_url,NETWORKID,senderid,temp,senderid,hum,senderid,bat,senderid,rssi,apikey);
  Serial.println(buf);
  return buf;
}

bool SendEmonData(String url)
{
  bool bRetVal=false;
  uint8_t http_Timeout = httpRetries;
  HTTPClient http;
  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) 
  {    
    Serial.print("Http begin");     
    while(http_Timeout-->0)
    { 

      http.begin(url);  //Specify request destination
      //Send the request
      int httpCode = http.GET();                                                                  
      if (httpCode > 0) 
      { //Check the returning code
   
        String payload = http.getString();   //Get the request response payload
        if(payload=="ok")
        {
            bRetVal=true;
            break;
        }
      }
      Serial.print(".");
    }
 
    http.end();   //Close connection
  }
  Serial.println();
  return bRetVal;
}


void loop() {
  float temp,hum,bat;


  if(((millis() - startMillis)>MeasureInterval_ms)||startMillis==0)
  {
    
    startMillis = millis();
  
    temp=get_sht31_temp();
    hum=get_sht31_hum();  
    bat=get_bat_voltage(); 

    if(SendEmonData(getEmonString(NODEID,temp,hum,bat,0)))
    {
      Serial.println("Emon Upload sucessful!");
    }
    else
    {
      Serial.println("Emon Upload failed!");
    }
    //Debug Values
    Serial.println();
    Serial.printf("Temp: %3.2f °C\r\n",temp);
    Serial.printf("Hum: %3.2f %\r\n",hum);
    Serial.printf("Bat: %3.2f V\r\n",bat);
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("RX 433MHz");
    display.printf("Tem:%3.1fC\r\n",temp);
    display.printf("Hum:%3.1f%%\r\n",hum);
    display.printf("Bat:%3.2fV\r\n",bat);
    display.display();
  }
  
  if(receive_radio())
  {
    uint32_t pak_rec=strtof(strtok((char *)data,","),NULL);
    float tem_rec=strtof(strtok(NULL,","),NULL);
    float hum_rec=strtof(strtok(NULL,","),NULL);
    float bat_rec=strtof(strtok(NULL,","),NULL);

    if(SendEmonData(getEmonString(senderId,tem_rec,hum_rec,bat_rec,rssi)))
    {
      Serial.println("Emon Upload sucessful!");
    }
    else
    {
      Serial.println("Emon Upload failed!");
    }
    //Data Received
  }



}
