
#include <SPI.h>
#include <ccspi.h> // ADDED
#include <string.h> //ADDED
#include<stdlib.h> // ADDED 
//#include <sha1.h>
#include <Adafruit_CC3000.h>
#include "utility/debug.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2); // you can change this clock speed
Adafruit_CC3000_Client client;         

#define WLAN_SSID       "DESIGNPROJECT"           // cannot be longer than 32 characters!
#define WLAN_PASS       "888222999"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2



#define SPRINKLER            6 //digital Pin 8
#define ONE_WIRE_BUS         7 //digital Pin 7
#define PELTIER              8 //digital Pin 6
#define HEATER               9 //digital Pin 9


OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
// DeviceAddress Thermometer = {0x28, 0xF6, 0x08, 0x3D, 0x05, 0x00, 0x00, 0x00};
DeviceAddress Thermometer = {0x28, 0x0D, 0xDD, 0x3C, 0x05, 0x00, 0x00, 0x89};
DallasTemperature sensors(&oneWire);

// Local server IP, port, and repository 
uint32_t ip = cc3000.IP2U32(192,168,1,208);
//uint32_t ip = cc3000.IP2U32(192,168,1,2);
int port = 80;


String repository = "/Vermi2/";
String result1;
String result2;
const int soilreading = A4;
int water_level;
float tempC; //temperature reading inside the bin

char sprBuffer[1];
char tempMax[2];
char tempMin[2];
int moisture;
int maxTemp;
int minTemp;
char getLo1[2];
char getLo2[2];
char getHi1[2];
char getHi2[2];

char buffer[10]; //used for float to string

//LiquidCrystal lcd(32, 30, 28, 26, 24, 22);

void setup(){ 
 // lcd.begin(16, 2);
  pinMode(soilreading, INPUT);
  pinMode(45, INPUT);
  pinMode(43, INPUT);
  pinMode(41, INPUT);
  pinMode(39, INPUT);
  pinMode(37, INPUT);
  pinMode(35, INPUT);
  pinMode(SPRINKLER, OUTPUT);
  digitalWrite(SPRINKLER, LOW);
  pinMode(PELTIER, OUTPUT);
  digitalWrite(PELTIER, LOW);
  pinMode(HEATER, OUTPUT);
  digitalWrite(HEATER,LOW);
  Serial.begin(115200);
  sensors.begin();
  sensors.setResolution(Thermometer, 12);
  

  // Set up CC3000 and get connected to the wireless network.
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }
  Serial.println();
  
}


void loop(void)
{                
          sprBuffer[0] = 'r';
          tempMin[0] = 'l';
          tempMax[0] = 'h';
          send_request_udp1();
          send_request_udp2();
          send_request_udp3(); 
          delay(10);
          
          water_level = digitalRead(45) + digitalRead(43) + digitalRead(41) + digitalRead(39) + digitalRead(37) +digitalRead(35);
          String waterlevel = String((int) water_level);
          sensors.requestTemperatures();
          tempC = sensors.getTempC(Thermometer);
          String temperature = floatToString(tempC); 

          moisture = analogRead(soilreading);
          moisture = constrain(moisture,250,900); // limit analogRead from range of 250 to 900
          moisture = map(moisture, 250, 900, 100, 0); // analogRead of a very wet condition of the soil is 250, and 900 for a very dry condition
          String soil = String((int) moisture);
          //get it in terms of percentage (0 = wet, 900 = dry)
         // String moistPercent = String((int) moisture) + "%";re
          String dataLCD = "Temp:  " + temperature + "                            " + "Soil:  " + soil + " %";
          String request = "GET "+ repository + "sensor.php?temp=" + temperature + "&soil=" + soil + "&water=" + waterlevel + " HTTP/1.0\r\nConnection: close\r\n\r\n";
          send_request(request);
          delay(700);
         Serial.print(dataLCD);
          automate();
          delay(10);
          
      }


bool send_request (String request) {
     
    // Connect    
    //Serial.println("Starting connection to server...");
    client = cc3000.connectTCP(ip, port);
    
    // Send request
    if (client.connected()) {
      client.println(request);      
      client.println(F(""));
      //Serial.println("Connected & Data sent");
    } 
    else {
      Serial.println(F("Connection failed"));    
    }

    while (client.connected()) {
      while (client.available()) {

      // Read answer
      char c1 = client.read();
     
      }
    }
    client.close();
    
   
}

void automate()
{
  if(maxTemp<=minTemp) {
    digitalWrite(PELTIER, LOW);
   digitalWrite(HEATER,LOW);}
 else if(tempC >= maxTemp){
  digitalWrite(PELTIER, HIGH);
  digitalWrite(HEATER, LOW);
}
  else if(tempC <= minTemp){
  digitalWrite(PELTIER, LOW);
  digitalWrite(HEATER, HIGH); 
  
}
  else{
    digitalWrite(PELTIER, LOW);
    digitalWrite(HEATER,LOW);
  }
 // Serial.println("MAX TEMP"); Serial.println(maxTemp);
 // Serial.println("MIN TEMP"); Serial.println(minTemp);
}

void send_request_udp1 () {
  
    client = cc3000.connectUDP(ip, 9999);

    if(client.connected()) {
    //  Serial.print(F("connected!\r\nIssuing request..."));
      
      // Assemble and issue request packet
      client.write(sprBuffer, sizeof(sprBuffer));

      if(client.available()) {
        client.read(sprBuffer, sizeof(sprBuffer));
        //Serial.println(sprBuffer);
          }
    
      if (sprBuffer[0] == '0') {
    digitalWrite(SPRINKLER,LOW);
  }
  
   if (sprBuffer[0] == '1') {
    digitalWrite(SPRINKLER,HIGH);
  }
      client.close();
    }
    
         
}

void send_request_udp2 () {
  
    client = cc3000.connectUDP(ip, 11111);

    if(client.connected()) {
    //  Serial.print(F("connected!\r\nIssuing request..."));
      
      // Assemble and issue request packet
      client.write(tempMin, sizeof(tempMin));

      if(client.available()) {
       
        client.read(tempMin, sizeof(tempMin));
        //Serial.println(tempMin);
        getLo1[0] = tempMin[0];
        getLo2[0] = tempMin[1];
        int t1 = atoi(getLo1);
        int t2 = atoi(getLo2);
        minTemp = (t1*10) + t2;
       
      }
      

      client.close();
    }
    
         
}
void send_request_udp3 () {
  
    client = cc3000.connectUDP(ip, 22222);

    if(client.connected()) {
    //  Serial.print(F("connected!\r\nIssuing request..."));
      
      // Assemble and issue request packet
      client.write(tempMax, sizeof(tempMax));

      if(client.available()) {
       
        client.read(tempMax, sizeof(tempMax));
        //Serial.println(tempMax);
        getHi1[0] = tempMax[0];
        getHi2[0] = tempMax[1];
        int t3 = atoi(getHi1);
        int t4 = atoi(getHi2);
        maxTemp = (t3*10) + t4;
      }
    
      client.close();
    }
    
         
}



String floatToString(float number) {
  //  dtostrf(floatVar, minStringWidthIncDecimalPoint, numVarsAfterDecimal, charBuf);
  dtostrf(number,5,2,buffer);
  return String(buffer);

}


