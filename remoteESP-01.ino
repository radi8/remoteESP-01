/* Version Date 2018-08-01 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

//#define FEATURE_Internet_Access



#define _DEBUG
#define _Version "0.9.0"
#define I2CAddressESPWifi 9

const char* ssid = "Jury Wireless";
const char* password = "je4lphHR";
const int sda = 0;
const int scl = 2;
const int rxPin = 3; //GPIO3
const int txPin = 1; //GPIO1
const int LED = 10;
unsigned int localtcpPort = 8475;  // local port to listen on

enum { // Receive commands from tcp client. Send commands to I2C slave.
  CMD_PWR_ON = 1, //Start the enum from 1
  CMD_PWR_OFF,
  CMD_RLY1_ON,    // HiQSDR
  CMD_RLY1_OFF,
  CMD_RLY2_ON,    //HermesLite
  CMD_RLY2_OFF,
  CMD_RLY3_ON,    // Linear
  CMD_RLY3_OFF,
  CMD_RLY4_ON,    // Tuner
  CMD_RLY4_OFF,   
  CMD_TUNE_DN,
  CMD_TUNE_UP,
  CMD_RADIO_0,    // No antenna selected
  CMD_RADIO_1,    // wire Antenna selected
  CMD_RADIO_2,
  CMD_RADIO_3,
  CMD_RADIO_4,
  CMD_READ_A0,    // Shack voltage
  CMD_READ_A1,
  CMD_READ_A2,
  CMD_READ_D2,    // Digital input via opto-coupler
  CMD_READ_D3,
  CMD_SET_LED_HI,
  CMD_SET_LED_LO,
  CMD_STATUS,
  CMD_ID // Always keep this last
};

enum { // Receive commands from remoteArduino (I2C slave). Send commands to tcp client.
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

String incomingPacket;  // buffer for incoming TCP/IC packets
char outgoingPacket[32];  // buffer for outgoing TCP/IP packets

/************************** Setup global I2C stuff **************************/
char I2C_sendBuf[32];
char I2C_recdBuf[32];

/************************** The constructor **************************/
void setup()
{
  // prepare GPIO2
  pinMode(rxPin, OUTPUT);      // sets the GPIO2 digital pin as output
  digitalWrite(rxPin, LOW);

  // Set up the serial port for tranamit only AT 115200
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  // Set up the I2C stuff 
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

  // Print the system info
  Serial.print("Program version: "); Serial.println(_Version);
  Serial.println();
  Serial.print("MAC Addr: "); Serial.println(WiFi.macAddress());
  Serial.print("IP Addr:  "); Serial.println(WiFi.localIP());
  Serial.print("Subnet:   "); Serial.println(WiFi.subnetMask());
  Serial.print("Gateway:  "); Serial.println(WiFi.gatewayIP());
  Serial.print("DNS Addr: "); Serial.println(WiFi.dnsIP());
  Serial.print("Channel:  "); Serial.println(WiFi.channel());
  Serial.print("Status: "); Serial.println(WiFi.status());

  // Start Server
  server.begin();
  Serial.printf("Now listening at IP %s, TCP port %d\n", WiFi.localIP().toString().c_str(), localtcpPort);
  delay(100);

  // Setup I2C stuff
  Wire.begin(sda, scl);//Set up as master
  Wire.setClockStretchLimit(40000);    // in µs
  Wire.onReceive(receiveEvent);

  memset(I2C_recdBuf, '\0', 32);
//   (CMD_ID, 28); // Command is 55 and response will be 17 characters
   
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

  // Data coming, wait until the client has finished sending it
#ifdef _DEBUG  
  Serial.println("new client");
#endif  
  
  while (!client.available()) {
    delay(1);
  }

  // Read the incoming data until '\r' is detected
  incomingPacket = client.readStringUntil('\r'); // Read into global variable
  
  client.flush();
  uint8_t cmdNumber = atoi(incomingPacket.c_str()); // Convert text to integer
#ifdef _DEBUG
  Serial.print("@Main Loop: cmdNumber value = "); Serial.println(cmdNumber);
#endif  
  processCmd(client, cmdNumber);
  delay(1);
  client.stop();
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

  switch (cmdNumber) {
    case CMD_PWR_ON:
      // Power on signal goes locally to this ESP01. This output drives an open drain FET
      // so going HIGH on turn-on signal drives the FET output from open drain to low.
      digitalWrite(rxPin, HIGH); // Turn Power on from ESP01 Rx pin
      sprintf(I2C_sendBuf, "%d\r", CMD_PWR_ON); // Borrow this buffer for TCP send
      client.print(I2C_sendBuf); // Echo the command back to tcp client
      break;      
    case CMD_PWR_OFF:
      // Power off signal goes locally to this ESP01
      digitalWrite(rxPin, LOW); // Turn Power off from ESP01 Rx pin
      sprintf(I2C_sendBuf, "%d\r", CMD_PWR_OFF); // Borrow this buffer for TCP send
      client.print(I2C_sendBuf); // Echo the command back to tcp client
      break;
    case CMD_RLY1_ON:
      sprintf(I2C_sendBuf, "%d", CMD_RLY1_ON);
      sendArduino(); // This proc adds '\r' to I2C_sendBuf string so we can simply
      client.print(I2C_sendBuf);  // send to TCP/IP client as ack message
      break;
    case CMD_RLY1_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_RLY1_OFF);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RLY2_ON:
      sprintf(I2C_sendBuf, "%d", CMD_RLY2_ON);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RLY2_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_RLY2_OFF);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RLY3_ON:
      sprintf(I2C_sendBuf, "%d", CMD_RLY3_ON);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RLY3_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_RLY3_OFF);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RLY4_ON:
      sprintf(I2C_sendBuf, "%d", CMD_RLY4_ON);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RLY4_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_RLY4_OFF);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_TUNE_DN: // Tune button clicked
      sprintf(I2C_sendBuf, "%d", CMD_TUNE_DN);
      sendArduino();
      client.print(I2C_sendBuf);
      Serial.print("@processCommand; case CMD_TUNE_DN, I2C_sendBuf = "); printBuffer(I2C_sendBuf);
      break;
    case CMD_TUNE_UP: // Tune button released
      sprintf(I2C_sendBuf, "%d", CMD_TUNE_UP);
      sendArduino();
      client.print(I2C_sendBuf);
      break;    
    case CMD_RADIO_0: // Foward radio buttons to Arduino for processing there
      sprintf(I2C_sendBuf, "%d", CMD_RADIO_0);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RADIO_1:
      sprintf(I2C_sendBuf, "%d", CMD_RADIO_1);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RADIO_2:
      sprintf(I2C_sendBuf, "%d", CMD_RADIO_2);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RADIO_3:
      sprintf(I2C_sendBuf, "%d", CMD_RADIO_3);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_RADIO_4:
      sprintf(I2C_sendBuf, "%d", CMD_RADIO_4);
      sendArduino();
      client.print(I2C_sendBuf);
      break;      
    case CMD_READ_A0:
      sendRequestCommand (CMD_READ_A0, 10); //processRequestResponse() adds 0x0D to I2C_recdBuf
      client.print(I2C_recdBuf);
      break;     
    case CMD_READ_A1:
      sendRequestCommand (CMD_READ_A1, 10);
      client.print(I2C_recdBuf);
      break;
    case CMD_READ_A2:
      sendRequestCommand (CMD_READ_A2, 10);
      client.print(I2C_recdBuf);
      Serial.print("@processCommand; case CMD_READ_A2, I2C_sendBuf = "); printBuffer(I2C_recdBuf);
      break;
    case CMD_READ_D2:
      sendRequestCommand (CMD_READ_D2, 8);
      client.print(I2C_recdBuf);
      break;
    case CMD_READ_D3:
      sendRequestCommand (CMD_READ_D3, 8);
      client.print(I2C_recdBuf);
      break;  
    case CMD_SET_LED_HI:
      sprintf(I2C_sendBuf, "%d", CMD_SET_LED_HI);
      sendArduino();
      client.print(I2C_sendBuf);
      break;
    case CMD_SET_LED_LO:
      sprintf(I2C_sendBuf, "%d", CMD_SET_LED_LO);
      sendArduino();
      client.print(I2C_sendBuf);
      break;

/*      
    case CMD_STATUS:
      sendRequestCommand (CMD_READ_A0, 7);
      sendToClient(client);
      sendRequestCommand (CMD_READ_A1, 7);
      sendToClient(client);
      sendRequestCommand (CMD_READ_A2, 7);
      sendToClient(client);
      sendRequestCommand (CMD_READ_D2, 4);
      sendToClient(client);
      sendRequestCommand (CMD_READ_D3, 4);
      sendToClient(client);
      break;
    case CMD_ID:
      sendRequestCommand (CMD_ID, 28);
      sendToClient(client);
      break;
*/
      
    default:
      // if nothing else matches, do the default
      // default is optional
      sprintf(I2C_sendBuf, "Invalid cmd: %d\r", cmdNumber);
      client.print(I2C_sendBuf);
      break;
  }
}

void printBuffer(char buffer[])
{
  uint8_t bufLen = strlen(buffer);

  for (uint8_t x=0; x<(bufLen+2); x++) {
    Serial.print(buffer[x], HEX); Serial.print(", ");
  }
  Serial.println();
}

/************************** I2C subroutines **************************/

void sendArduino()
// Sends the data contained in I2C_sendBuf[] to the Arduino slave. The caller needs
// to ensure that the string is properly formatted.The routine ensures it is properly
// terminated and sends it via I2C.
{
  uint8_t len;

  len = strlen(I2C_sendBuf);
  I2C_sendBuf[len] = '\r';  // Add a carriage return terminator

  Wire.beginTransmission(I2CAddressESPWifi);
  for (uint8_t x = 0; x <= len; x++) {
    Wire.write(I2C_sendBuf[x]);
  }
  Wire.endTransmission();
#ifdef _DEBUG
  Serial.print("@sendArduino(): Value sent in I2C_sendBuf[] = "); printBuffer(I2C_sendBuf);
#endif  
}

void receiveEvent(int howMany) {
  memset(I2C_recdBuf, '\0', 32);
  for (byte i = 0; i < howMany; i++)
  {
    I2C_recdBuf[i] = Wire.read ();
  }  // end of for loop
#ifdef _DEBUG
  Serial.print("@receiveEvent(): Value received in I2C_recdBuf = "); printBuffer(I2C_recdBuf);
#endif  
}

void sendRequestCommand(const byte cmd, const int responseSize)
// Process a command from the tcp client to the slave via ESP01 which expects some data to be
// returned from the slave and in turn sent back to the tcp client. Enter with the command
// from the tcp client passed in the cmd variable and place into I2C_sendBuf[] The responseSize
// variable is used later by "Wire.requestFrom(I2Caddress, responseSize)" to tell the Arduino
// slave how many characters to send back when it does its response to the requestFrom.
{
  uint8_t len;

  delay(10);
  sprintf(I2C_sendBuf, "%d", cmd); // Convert the command to a string
  Serial.print("@sendRequestCommand: cmd = "); Serial.println(cmd); //debug

  sendArduino();
#ifdef _DEBUG  
  Serial.print("@sendRequestCommand: Cnd sent in I2C_sendBuf = "); printBuffer(I2C_sendBuf);
#endif
// Having sent a command on to the Arduino, we now request a response from it with the data
// it has been programmed to get from receiving this command. We provide it with a response
// size which is big enough to hold the data it is going to send back. It is OK to be too
// big as we will clean off any trailing rubbish (shows as bytes of 0xFF)
  if (Wire.requestFrom (I2CAddressESPWifi, responseSize)) { // Ask foresponse from Arduino
#ifdef _DEBUG    
    Serial.print("@sendRequestCommand; Value before processRequestResponse in I2C_recdBuf = ");
    printBuffer(I2C_recdBuf);
#endif
    processRequestResponse();  // Get the requested response and clean off any rubbish
  } else {
    Serial.println("Wire.requestFrom() failure; maybe slave wasn't ready or not connected");
  }
  // we return with the request response in the global "char I2C_recdBuf[32]" The caller will be
  // responsible for sending the response up the line to tcp client.
} // end of sendRequestCommand


void processRequestResponse()
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
  }   // end of for loop 
  
  // This should leave I2C_recdBuf[] with the string followed 
  // by NULLs. We need to find the first null and replace it with '\r'
  I2C_recdBuf[strlen(I2C_recdBuf)] = '\r';  // Add a carriage return terminator
  
#ifdef _DEBUG
  Serial.print("@processRequestResponse(): Value plac
  ed in I2C_recdBuf[] = "); printBuffer(I2C_recdBuf);
#endif
}

