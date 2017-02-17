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

enum { // Commands received from remote
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

enum { // Arduino sends these, pass on to tcp client
  _pwrSwitch = 1, //Start the emun from 1
  _tuneState,
  _volts,
  _amps,
  _analog2,
  _digital2,
  _digital3,
  _rly1,
  _rly2,
  _antenna,
  _message
};

/************************** Setup global TCP stuff **************************/
WiFiServer server(localtcpPort);
// NETWORK: Static IP details...
IPAddress ip(192, 168, 1, 70);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
#ifdef FEATURE_Internet_Access
IPAddress dns(202.180.64.10);
#endif

String incomingPacket;  // buffer for incoming packets
//char  replyPacket[] = "Hi there! Got the message :-)";  // a reply string to send back

/************************** Setup global I2C stuff **************************/
char I2C_sendBuf[32];
char I2C_recdBuf[32];


void setup()
{
  // prepare GPIO2
  pinMode(rxPin, OUTPUT);      // sets the GPIO2 digital pin as output
  digitalWrite(rxPin, HIGH);

  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println();

  /************************** Setup TCP stuff **************************/
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

  /************************** Setup I2C stuff **************************/
  Wire.begin(sda, scl);//Set up as master
  Wire.setClockStretchLimit(100000);    // in µs
  Wire.onReceive(receiveEvent);

  memset(I2C_recdBuf, '\0', 32);
  sendCommand (CMD_ID, 28); // Command is 55 and response will be 17 characters
  /*
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
  */
} // end of setup

/************************** Main Loop **************************/

void loop()
// Run through the loop checking for a tcp client connection and read
// the command sent. The I2C traffic is interrupt driven
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
  incomingPacket = client.readStringUntil('\r'); // Read into global variable
  Serial.println(incomingPacket);
  client.flush();
  uint8_t cmdNumber = atoi(incomingPacket.c_str()); // Convert text to integer

  processCmd(client, cmdNumber);
  delay(1);
}

/************************** Subroutines start here **************************/

void processCmd(WiFiClient &client, uint8_t cmdNumber)
// Process a command sent via TCP from remote. A response may be sent back
// to the tcp client or handed on via I2C to the Arduino or both. Where
// appropriate the arduino will send data back to the tcp client via I2C.
// The global "String incomingPacket" still holds the original command text.
// Strings to be sent are placed in I2C_sendBuf and sent with sendArduino();
{
  int  x;
  char  cmdBuffer[10]; // Holds the command to be sent to the client
  char cmdArg[5]; // Holds value to be appended to cmdBuffer and sent
  // with: client.print(cmdBuffer);

  switch (cmdNumber) {
    case CMD_PWR_ON:
      // Send power on signal via I2C to Arduino
      digitalWrite(rxPin, LOW); // Turn Power on from ESP01 Rx pin
      itoa(_pwrSwitch, cmdBuffer, 10);
      strcpy(cmdArg, " 1");
      strcat(cmdBuffer, cmdArg);
      client.print(cmdBuffer); // Echo the command back to tcp client
      break;
    case CMD_PWR_OFF:
      digitalWrite(rxPin, HIGH); // Turn Power off from ESP01 Rx pin
      itoa(_pwrSwitch, cmdBuffer, 10);
      strcpy(cmdArg, " 0");
      strcat(cmdBuffer, cmdArg);
      client.print(cmdBuffer); // Echo the command back to tcp client
      break;
    case CMD_TUNE: // Tune button clicked
      Serial.println("ESP01 has received 03 command");
      strcpy(I2C_sendBuf, "3");
      itoa(CMD_TUNE, cmdBuffer, 10); // Send this command on to the arduino
      client.print(cmdBuffer); // Echo the command back to tcp client
      sendArduino();
      break;
    case CMD_READ_A0:
      //      itoa(CMD_READ_A0, I2C_sendBuf, 10); // Send this command on to the arduino
      sendCommand (CMD_READ_A0, 7);
      client.println(I2C_recdBuf);
      break;
    case CMD_READ_A1:
      //      itoa(CMD_READ_A1, I2C_sendBuf, 10); // Send this command on to the arduino
      sendCommand (CMD_READ_A1, 7);
      client.println(I2C_recdBuf);
      break;
    case CMD_READ_A2:
      //      itoa(CMD_READ_A2, I2C_sendBuf, 10); // Send this command on to the arduino
      sendCommand (CMD_READ_A2, 7);
      client.println(I2C_recdBuf);
      break;
    case CMD_READ_D2:
      //      itoa(CMD_D2_HI, I2C_sendBuf, 10); // Send this command on to the arduino
      sendCommand (CMD_READ_D2, 4);
      client.println(I2C_recdBuf);
      break;
    case CMD_READ_D3:
      //      itoa(CMD_D2_LO, I2C_sendBuf, 10); // Send this command on to the arduino
      sendCommand (CMD_READ_D3, 4);
      client.println(I2C_recdBuf);
      break;
    case CMD_SET_RLY1_ON:
      itoa(CMD_SET_RLY1_ON, I2C_sendBuf, 10); // Send this command on to the arduino
      sendArduino();
      break;
    case CMD_SET_RLY1_OFF:
      itoa(CMD_SET_RLY1_OFF, I2C_sendBuf, 10); // Send this command on to the arduino
      sendArduino();
      break;
    case CMD_SET_RLY2_ON:
      itoa(CMD_SET_RLY2_ON, I2C_sendBuf, 10); // Send this command on to the arduino
      sendArduino();
      break;
    case CMD_SET_RLY2_OFF:
      itoa(CMD_SET_RLY2_OFF, I2C_sendBuf, 10); // Send this command on to the arduino
      sendArduino();
      break;
    case CMD_SET_LED_HI:
      itoa(CMD_SET_LED_HI, I2C_sendBuf, 10); // Send this command on to the arduino
      sendArduino();
      break;
    case CMD_SET_LED_LO:
      itoa(CMD_SET_LED_LO, I2C_sendBuf, 10); // Send this command on to the arduino
      sendArduino();
      break;
    case CMD_STATUS:
      itoa(CMD_STATUS, I2C_sendBuf, 10); // Send this command on to the arduino
      sendArduino();
      break;
    case CMD_ID:
      //      itoa(CMD_ID, I2C_sendBuf, 10); // Send this command on to the arduino
      sendCommand (CMD_ID, 28);
      client.println(I2C_recdBuf);
      break;
    default:
      // if nothing else matches, do the default
      // default is optional
      client.println("@ESP09 processCmd: " + incomingPacket + " command Received");
      break;
  }
}


/************************** I2C subroutines **************************/

void requestFromResponse()
{
  int x = 0;

  memset(I2C_recdBuf, '\0', 32);
  while (Wire.available()) {
    I2C_recdBuf[x] = Wire.read();
    x++;
  }  // end of for loop
  x = strlen(I2C_recdBuf);
  I2C_recdBuf[x] = '\0';
  Serial.print("@requestFromResponse(): I2C_recdBuf[] contents = ");
  Serial.print(I2C_recdBuf);
  Serial.print(", and x = ");
  Serial.println(x);
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

  Serial.print("@receiveEvent(): I2C_recdBuf = ");
  Serial.println(I2C_recdBuf);
}

void sendCommand (const byte cmd, const int responseSize)
// Process a command from the tcp client to the slave via ESP01 which
// expects some data to be returned from the slave and in turn sent
// back to the tcp client. Enter with the command from the tcp client
// passed in the cmd variable and place into I2C_sendBuf[]
{
  uint8_t len;

  delay(10);
  sprintf(I2C_sendBuf, "%d", cmd); // Convert the command to a string
  len = strlen(I2C_sendBuf);
  I2C_sendBuf[len] = '\0';
  Wire.beginTransmission (I2CAddressESPWifi); // send to I2C slave
  for (byte i = 0; i <= len; i++)
  {
    Wire.write(I2C_sendBuf[i]); // Chug out 1 character at a time
  }  // end of for loop
  Wire.endTransmission ();
  Serial.print("Sent from ESP01:sendCommand() ");
  Serial.println(I2C_sendBuf);
  if (Wire.requestFrom (I2CAddressESPWifi, responseSize)) {
    requestFromResponse(); // Get the response from the slave into recdBuf[32]
  } else {
    Serial.println("Wire.requestFrom() failure; maybe slave wasn't ready or not connected");
  }
  // Caller will send the response up the line to tcp client
} // end of sendCommand
