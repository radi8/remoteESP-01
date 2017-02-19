#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

//#define FEATURE_Internet_Access
#define _DEBUG
#define _Version "0.8.0"
#define I2CAddressESPWifi 9

const char* ssid = "Guest";
const char* password = "BeMyGuest";
const int sda = 0;
const int scl = 2;
const int rxPin = 3; //GPIO3
const int txPin = 1; //GPIO1
const int LED = 10;
unsigned int localtcpPort = 8475;  // local port to listen on

enum { // Receive commands from tcp client. Send commands to I2C slave.
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
  CMD_ID // Always keep this last
};

enum { // Receive commands from I2C slave. Send commands to tcp client.
  _pwrSwitch = CMD_ID + 1,
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
  Serial.println(_Version);

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
  Wire.setClockStretchLimit(40000);    // in µs
  Wire.onReceive(receiveEvent);

  memset(I2C_recdBuf, '\0', 32);
   (CMD_ID, 28); // Command is 55 and response will be 17 characters
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

  // Data coming, wait until the client sending it
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  incomingPacket = client.readStringUntil('\r'); // Read into global variable
#ifdef _DEBUG
  Serial.print("@Main Loop: incoming packet value = ");
  Serial.println(incomingPacket);
#endif  
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

#ifdef _DEBUG
  Serial.print("@ESP01 processCmd: cmdNumber = ");
  Serial.println(cmdNumber);
#endif  
  switch (cmdNumber) {
    case CMD_PWR_ON:
      // Power on signal goes locally to this ESP01
      digitalWrite(rxPin, LOW); // Turn Power on from ESP01 Rx pin
      sprintf(I2C_recdBuf, "%d 1", _pwrSwitch);
      sendToClient(client); // Echo the command back to tcp client
      break;
    case CMD_PWR_OFF:
      // Power off signal goes locally to this ESP01
      digitalWrite(rxPin, HIGH); // Turn Power off from ESP01 Rx pin
      sprintf(I2C_recdBuf, "%d 0", _pwrSwitch);
      sendToClient(client); // Echo the command back to tcp client
//      client.print(cmdBuffer); // Echo the command back to tcp client
      break;
    case CMD_TUNE: // Tune button clicked
      sprintf(cmdBuffer, "%d", CMD_TUNE);
      strcpy(I2C_sendBuf, cmdBuffer);
      client.print(cmdBuffer); // Echo the command back to tcp client
      sendArduino();
      break;
    case CMD_READ_A0:
      sendCommand (CMD_READ_A0, 7);
      sendToClient(client);
      break;
    case CMD_READ_A1:
      sendCommand (CMD_READ_A1, 7);
      sendToClient(client);
      break;
    case CMD_READ_A2:
      sendCommand (CMD_READ_A2, 7);
      sendToClient(client);
      break;
    case CMD_READ_D2:
      sendCommand (CMD_READ_D2, 4);
      sendToClient(client);
      break;
    case CMD_READ_D3:
      sendCommand (CMD_READ_D3, 4);
      sendToClient(client);
      break;
    case CMD_SET_RLY1_ON:
      sprintf(I2C_sendBuf, "%d", CMD_SET_RLY1_ON);
      sendArduino();
      break;
    case CMD_SET_RLY1_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_SET_RLY1_OFF);
      sendArduino();
      break;
    case CMD_SET_RLY2_ON:
      sprintf(I2C_sendBuf, "%d", CMD_SET_RLY2_ON);
      sendArduino();
      break;
    case CMD_SET_RLY2_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_SET_RLY2_OFF);
      sendArduino();
      break;
    case CMD_SET_LED_HI:
      sprintf(I2C_sendBuf, "%d", CMD_SET_LED_HI);
      sendArduino();
      break;
    case CMD_SET_LED_LO:
      sprintf(I2C_sendBuf, "%d", CMD_SET_LED_LO);
      sendArduino();
      break;
    case CMD_STATUS:
      sendCommand (CMD_READ_A0, 7);
      sendToClient(client);
      sendCommand (CMD_READ_A1, 7);
      sendToClient(client);
      sendCommand (CMD_READ_A2, 7);
      sendToClient(client);
      sendCommand (CMD_READ_D2, 4);
      sendToClient(client);
      sendCommand (CMD_READ_D3, 4);
      sendToClient(client);
      break;
    case CMD_ID:
      sendCommand (CMD_ID, 28);
      sendToClient(client);
      break;
    default:
      // if nothing else matches, do the default
      // default is optional
      client.println("@ESP09 processCmd: " + incomingPacket +
      " Invalid command Received");
      break;
  }
}

void sendToClient(WiFiClient &client)
// Simply send the contents of I2C_recdBuf[] to the tcp client
{
  client.print(I2C_recdBuf);
#ifdef _DEBUG
  Serial.print("@sendToClient(): Data sent = ");
  Serial.println(I2C_recdBuf);
#endif  
}

/************************** I2C subroutines **************************/

void requestFromResponse()
// Process a response from the slave to a request from ESP01 master for a
// value from the slave. The value of the response is placed in I2C_recdBuf[]
{
  int x = 0;

// A fudge is done here as wire.read returns 0xFF when reading beyond the length
// of the sent string e.g. reading 5 bytes of a string containing cat will
// return c, a, t, 0xFF, 0xFF rather than c, a, t, 0x00, Random byte.
  memset(I2C_recdBuf, NULL, sizeof(I2C_recdBuf));
  while (Wire.available()) {
    char c = Wire.read();
    if((c != 0xFF) && (c != NULL)) {
      I2C_recdBuf[x] = c;
    } else {
      I2C_recdBuf[x] = NULL;
    }
    x++;
  }  // end of for loop

#ifdef _DEBUG
  Serial.print("@requestFromResponse(): I2C_recdBuf[] = ");
  Serial.println(I2C_recdBuf);
#endif
}

void sendArduino()
// Sends the data in I2C_sendBuf[] to slave. The caller needs to ensure that
// the string is properly formatted.The routine ensures it is properly
// terminated and sends it.
{
  uint8_t len;

  len = strlen(I2C_sendBuf);
  I2C_sendBuf[len] = '\0';
 
  Wire.beginTransmission(I2CAddressESPWifi);
  for (uint8_t x = 0; x <= len; x++) {
    Wire.write(I2C_sendBuf[x]);
  }
  Wire.endTransmission();
#ifdef _DEBUG
  Serial.print("@sendArduino(): I2C_sendBuf = ");
  Serial.print(I2C_sendBuf);
  Serial.println(", written to slave");
#endif  
}

void receiveEvent(int howMany) {
  memset(I2C_recdBuf, '\0', 32);
  for (byte i = 0; i < howMany; i++)
  {
    I2C_recdBuf[i] = Wire.read ();
  }  // end of for loop
#ifdef _DEBUG
  Serial.print("@receiveEvent(): I2C_recdBuf = ");
  Serial.println(I2C_recdBuf);
#endif  
}

void sendCommand(const byte cmd, const int responseSize)
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
#ifdef _DEBUG  
  Serial.print("@sendCommand(): Sent to slave = ");
  Serial.println(I2C_sendBuf);
#endif  
  if (Wire.requestFrom (I2CAddressESPWifi, responseSize)) {
    requestFromResponse(); // Get the response from the slave into recdBuf[32]
  } else {
    Serial.println("Wire.requestFrom() failure; maybe slave wasn't ready or not connected");
  }
  // Caller will send the response up the line to tcp client
} // end of sendCommand
