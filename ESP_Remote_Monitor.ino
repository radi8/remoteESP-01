#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

//#define FEATURE_Internet_Access

#define I2CAddressESPWifi 8

const char* ssid = "Guest";
const char* password = "BeMyGuest";
const int sda = 0;
const int scl = 2;
const int rxPin = 3; //GPIO3
const int txPin = 1; //GPIO1

WiFiUDP Udp;
// NETWORK: Static IP details...
IPAddress ip(192, 168, 1, 70);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
#ifdef FEATURE_Internet_Access
  IPAddress dns(202.180.64.10);
#endif

unsigned int localUdpPort = 8475;  // local port to listen on

char incomingPacket[255];  // buffer for incoming packets
char  replyPacket[] = "Hi there! Got the message :-)";  // a reply string to send back


void setup()
{
  pinMode(rxPin, OUTPUT);      // sets the IO2 digital pin as output
  digitalWrite(rxPin, HIGH);

  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
  Serial.println();
  Wire.begin(sda, scl);//Change to Wire.begin() for non ESP.

  Serial.printf("Connecting to %s/%d ", ssid, localUdpPort);
// Static IP Setup Info Here...
#ifdef FEATURE_Internet_Access
  // If you need Internet Access You should Add DNS also...
  WiFi.config(ip, dns, gateway, subnet);
#else
  WiFi.config(ip, gateway, subnet);
#endif

// Start Server
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}


void loop()
{
  uint8_t cmd;
  int packetSize = Udp.parsePacket();
  
  if (packetSize) // Process incoming packets if we received some
  {
    // receive incoming UDP packets
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;  //Terminate string
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);
/*
    // send back a reply, to the IP address and port we got the packet from
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(replyPacket);
    Udp.endPacket();
*/    
    cmd = getCmd(incomingPacket);
    Serial.printf("cmd value = %d\n", cmd);
    if (cmd > 0) processCmd(cmd);
  }
}

uint8_t getCmd(char* incomingPacket)
{
  long cmdNumber;
  char * pEnd;

  cmdNumber = strtol(incomingPacket, &pEnd, 10);
  return cmdNumber;
}

void processCmd(uint8_t cmd)
{
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  switch (cmd) {
    case 1:
      // Send power on signal via I2C to Arduino
      digitalWrite(rxPin,LOW); // Turn Power on    
      Udp.write("03 1");    
      break;
    case 2:
      digitalWrite(rxPin,HIGH); // Turn Power off
      Udp.write("03 0");
      break;
    case 3:
      // Add code here
      break;
    case 4:
      // Add code here
      break;      
    case 5: //debug note: This code should switch a relay via Arduino I2C
      Udp.write("01 12600");
      break;
    case 6:
      Udp.write("01 12600");
      break;      
    case 7:
      Udp.write("01 0");
      break;
    case 8:
      Udp.write("01 0");
      break;
    default: 
      // if nothing else matches, do the default
      // default is optional
    break;
  }
  Udp.endPacket();
// send back a reply, to the IP address and port we got the packet from  
}

