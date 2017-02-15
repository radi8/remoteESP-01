#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

//#define FEATURE_Internet_Access

#define I2CAddressESPWifi 9

const char* ssid = "Guest";
const char* password = "BeMyGuest";
const int sda = 0;
const int scl = 2;
const int rxPin = 3; //GPIO3
const int txPin = 1; //GPIO1
const int LED = 10;
unsigned int localtcpPort = 8475;  // local port to listen on

enum {
  CMD_PWR_ON = 1, //Start the enum from 1
  CMD_PWR_OFF,
  CMD_TUNE,
  CMD_READ_A0,
  CMD_READ_A1,
  CMD_READ_A2,
  CMD_READ_D2,
  CMD_READ_D3,
  CMD_SET_RLY1_ON,
  CMD_SET_RLY1_OFF,
  CMD_SET_RLY2_ON,
  CMD_SET_RLY2_OFF,
  CMD_SET_LED_HI,
  CMD_SET_LED_LO,
  CMD_STATUS,
  CMD_ID
};

WiFiServer server(localtcpPort);
// NETWORK: Static IP details...
IPAddress ip(192, 168, 1, 70);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
#ifdef FEATURE_Internet_Access
IPAddress dns(202.180.64.10);
#endif



char incomingPacket[255];  // buffer for incoming packets
char  replyPacket[] = "Hi there! Got the message :-)";  // a reply string to send back

char I2C_sendBuf[32];
char I2C_recdBuf[32];


void setup()
{
  // prepare GPIO2
  pinMode(rxPin, OUTPUT);      // sets the GPIO2 digital pin as output
  digitalWrite(rxPin, HIGH);

  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println();
  Wire.begin(sda, scl);//Set up as master
  Wire.setClockStretchLimit(100000);    // in µs
  Wire.onReceive(receiveEvent);

  Serial.printf("Connecting to %s/%d ", ssid, localtcpPort);
  // Static IP Setup Info Here...
#ifdef FEATURE_Internet_Access
  // If you need Internet Access You should Add DNS also...
  WiFi.config(ip, dns, gateway, subnet);
#else
  WiFi.config(ip, gateway, subnet);
#endif

  // Connect to WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
  Serial.print("MAC Addr: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP Addr:  ");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet:   ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway:  ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS Addr: ");
  Serial.println(WiFi.dnsIP());
  Serial.print("Channel:  ");
  Serial.println(WiFi.channel());
  Serial.print("Status: ");
  Serial.println(WiFi.status());

  // Start Server
  server.begin();
  Serial.printf("Now listening at IP %s, TCP port %d\n", WiFi.localIP().toString().c_str(), localtcpPort);
  delay(100);

  memset(I2C_recdBuf, '\0', 32);
  sendCommand (CMD_ID, 28); // Command is 55 and response will be 17 characters
  delay(100);//Wait for Slave to calculate response.
  int x = Wire.available();
  Serial.print("@Slave setup routine: Wire.available() = ");
  Serial.println (x);
  if (x) {

    for (byte i = 0; i < x ; i++) {
      I2C_recdBuf[i] = Wire.read ();
    }  // end of for loop
    Serial.println();
    Serial.print("@Slave setup routine: I2C_recdBuf[] contents = ");
    Serial.println (I2C_recdBuf);
  }
  else Serial.println ("No response to ID request");
} // end of setup


void loop()
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  int val;
  if (req == "1")
    val = LOW;
  else if (req == "2")
    val = HIGH;
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }

  // Set GPIO2 according to the request
  digitalWrite(rxPin, val);

  client.flush();

  // Prepare the response
/*  
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
  s += (val) ? "high" : "low";
  s += "</html>\n";
*/
  String s = "GPIO is now ";
  s += (val) ? "high" : "low";
  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disconnected");

  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed
}

uint8_t getCmd(char* incomingPacket)
{
  long cmdNumber;
  char * pEnd;

  cmdNumber = strtol(incomingPacket, &pEnd, 10);
  return cmdNumber;
}
/*
  void processCmd(uint8_t cmd)
  // Process a command sent via UDP from remote
  {
  int  x;
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  switch (cmd) {
    case 1:
      // Send power on signal via I2C to Arduino
      digitalWrite(rxPin, LOW); // Turn Power on
      Udp.write("03 1");
      break;
    case 2:
      digitalWrite(rxPin, HIGH); // Turn Power off
      Udp.write("03 0");
      break;
    case 3: // Tune button clicked
      Serial.println("ESP01 has received 03 command");
      Udp.write("03 received at ESP01");
      strcpy(I2C_sendBuf, "1");
      sendArduino();
      break;
    case 4:
      strcpy(I2C_sendBuf, "0");
      sendArduino(); // Hello button clicked
      break;
    case 5: //debug note: This code should switch a relay via Arduino I2C
      Udp.write("01 12600");
      sendCommand (CMD_READ_A1, 4);
      requestFromResponse();
      break;
    case 6:
      Udp.write("01 12600");
      sendCommand (CMD_READ_A2, 8);
      requestFromResponse();
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
*/
/************************** I2C subroutines **************************/

void requestFromResponse()
{
  int x;

  delay(10);//Wait for Slave to calculate response.
  memset(I2C_recdBuf, '\0', 32);
  x = Wire.available();
  //  Serial.print("@requestFromResponse(): Wire.available() = ");
  //  Serial.println (x);
  if (x) {
    for (byte i = 0; i < x ; i++) {
      I2C_recdBuf[i] = Wire.read ();
    }  // end of for loop
    Serial.print("@requestFromResponse(): I2C_recdBuf[] contents = ");
    Serial.println (I2C_recdBuf);
  }
}

void sendArduino()
{
  uint8_t len;

  len = strlen(I2C_sendBuf);
  I2C_sendBuf[len] = '\0';

  Wire.beginTransmission(I2CAddressESPWifi);
  for (uint8_t x = 0; x <= len; x++) {
    Wire.write(I2C_sendBuf[x]);
  }
  Wire.endTransmission();
}

void receiveEvent(int howMany) {
  memset(I2C_recdBuf, '\0', 32);
  for (byte i = 0; i < howMany; i++)
  {
    I2C_recdBuf[i] = Wire.read ();
  }  // end of for loop

  Serial.println(I2C_recdBuf);
}

void sendCommand (const byte cmd, const int responseSize)
{
  uint8_t len;

  delay(10);
  sprintf(I2C_sendBuf, "%d", cmd);
  len = strlen(I2C_sendBuf);
  I2C_sendBuf[len] = '\0';
  Wire.beginTransmission (I2CAddressESPWifi);
  for (byte i = 0; i <= len; i++)
  {
    Wire.write(I2C_sendBuf[i]); // Chug out 1 character at a time
  }  // end of for loop
  Wire.endTransmission ();
  Serial.print("Sent from ESP01:sendCommand() ");
  Serial.println(I2C_sendBuf);
  Wire.requestFrom (I2CAddressESPWifi, responseSize);
}  // end of sendCommand
