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
  CMD_ANT_1,      // No antenna selected (Dummy Load)
  CMD_ANT_2,      // Wire
  CMD_ANT_3,      // Mag Loop
  CMD_ANT_4,      // LoG
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

  // Set up the serial port for transmit only, at 115200
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
//  Wire.onReceive(receiveEvent);

  memset(I2C_recdBuf, '\0', 32);
   
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
  processCmd(cmdNumber);
  
  // Now send response from Arduino slave to remote client.
  // The response is in I2C_sendBuf[] and is '\r terminated'
  client.print(I2C_sendBuf); // Echo the response back to tcp client
  
  delay(1);
  client.stop();
}

/************************** Subroutines start here **************************/

void processCmd(uint8_t cmdNumber)
// Process a command sent via TCP from remote. A response will be sent back
// to the tcp client which is received via I2C from the Arduino slave.

// CMD numbers are converted to a string and sent to the Arduino slave by
// placing them into 'I2C_sendBuf' and sending with sendArduino() subroutine;
{
  int  x;

  switch (cmdNumber) {
    case CMD_PWR_ON:
      sprintf(I2C_sendBuf, "%d", CMD_PWR_ON); // Borrow this buffer for TCP send
      sendArduino(10);
      break;
    case CMD_PWR_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_PWR_OFF); // Borrow this buffer for TCP send
      sendArduino(10);
      break;
    case CMD_RLY1_ON:
      sprintf(I2C_sendBuf, "%d", CMD_RLY1_ON);
      sendArduino(10);
      break;
    case CMD_RLY1_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_RLY1_OFF);
      sendArduino(10);
      break;
    case CMD_RLY2_ON:
      sprintf(I2C_sendBuf, "%d", CMD_RLY2_ON);
      sendArduino(10);
      break;
    case CMD_RLY2_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_RLY2_OFF);
      sendArduino(10);
      break;
    case CMD_RLY3_ON:
      sprintf(I2C_sendBuf, "%d", CMD_RLY3_ON);
      sendArduino(10);
      break;
    case CMD_RLY3_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_RLY3_OFF);
      sendArduino(10);
      break;
    case CMD_RLY4_ON:
      sprintf(I2C_sendBuf, "%d", CMD_RLY4_ON);
      sendArduino(10);
      break;
    case CMD_RLY4_OFF:
      sprintf(I2C_sendBuf, "%d", CMD_RLY4_OFF);
      sendArduino(10);
      break;
    case CMD_TUNE_DN: // Tune button clicked
      sprintf(I2C_sendBuf, "%d", CMD_TUNE_DN);
      sendArduino(10);
      break;
    case CMD_TUNE_UP: // Tune button released
      sprintf(I2C_sendBuf, "%d", CMD_TUNE_UP);
      sendArduino(10);
      break;
    case CMD_ANT_1:
      sprintf(I2C_sendBuf, "%d", CMD_ANT_1);
      sendArduino(10);
      break;
    case CMD_ANT_2:
      sprintf(I2C_sendBuf, "%d", CMD_ANT_2);
      sendArduino(10);
      break;
    case CMD_ANT_3:
      sprintf(I2C_sendBuf, "%d", CMD_ANT_3);
      sendArduino(10);
      break;
    case CMD_ANT_4:
      sprintf(I2C_sendBuf, "%d", CMD_ANT_4);
      sendArduino(10);
      break;
    case CMD_READ_A0:
      sprintf(I2C_sendBuf, "%d", CMD_READ_A0);
      sendArduino(10);
      break;
    case CMD_READ_A1:
      sprintf(I2C_sendBuf, "%d", CMD_READ_A1);
      sendArduino(10);
      break;
    case CMD_READ_A2:
      sprintf(I2C_sendBuf, "%d", CMD_READ_A2);
      sendArduino(10);
      break;
    case CMD_READ_D2:
      sprintf(I2C_sendBuf, "%d", CMD_READ_D2);
      sendArduino(10);
      break;
    case CMD_READ_D3:
      sprintf(I2C_sendBuf, "%d", CMD_READ_D3);
      sendArduino(10);
      break;
    case CMD_SET_LED_HI:
      sprintf(I2C_sendBuf, "%d", CMD_SET_LED_HI);
      sendArduino(10);
      break;
    case CMD_SET_LED_LO:
      sprintf(I2C_sendBuf, "%d", CMD_SET_LED_LO);
      sendArduino(10);
      break;
    case CMD_STATUS:
      sprintf(I2C_sendBuf, "%d", CMD_STATUS);
      sendArduino(10);
      break;
    case CMD_ID:
      sprintf(I2C_sendBuf, "%d", CMD_ID);
      sendArduino(10);
      break;
    default:
      // if nothing else matches, do the default
      // default is optional
      sprintf(I2C_sendBuf, "Invalid cmd: %d\r", cmdNumber);
      Serial.println(I2C_sendBuf);
      break;
  }
}

void printBuffer(char buffer[])
{
  uint8_t bufLen = strlen(buffer);

  for (uint8_t x = 0; x < (bufLen + 2); x++) {
    Serial.print(buffer[x], HEX); Serial.print(", ");
  }
  Serial.println();
}

/************************** I2C subroutines **************************/

void sendArduino(int bytesExpected)
// Sends the data contained in I2C_sendBuf[] to the Arduino slave. The caller needs
// to ensure that the string is properly formatted. This routine ensures it is
// properly terminated and sends it via I2C.The slave returns the response as a text
// string which is concatenated to the command value using the recBuff[].
{
  uint8_t len;
  int cmdValue = 0;

  len = strlen(I2C_sendBuf);
  I2C_sendBuf[len] = '\r';  // Add a carriage return terminator

  // clear the recBuff[]
  memset(I2C_recdBuf, '\0', 32);
//  strcpy(I2C_recdBuf, I2C_sendBuf);
//  I2C_recdBuf[len] = ' ';

  Wire.beginTransmission(I2CAddressESPWifi);
  for (uint8_t x = 0; x <= len; x++) {
    Wire.write(I2C_sendBuf[x]);
  }
  Wire.endTransmission();

//  I2C_sendBuf[len] = ' '; // A space ready to concatenate the response
  // Now get the response by requesting a reading from the remote Arduino
  Wire.requestFrom(I2CAddressESPWifi, bytesExpected);
//  len++;
  memset(I2C_sendBuf, '\0', 32); // Clear the buffer ready for response from slave
  len = 0;
  while (Wire.available()) { // slave may send less than requested
    I2C_sendBuf[len] = Wire.read(); // receive a byte as character
    len++;
  }
  I2C_sendBuf[len] = '\r';
//  Serial.println(I2C_sendBuf);   // print the string to be sent to remote client via wifi
#ifdef _DEBUG
  Serial.print("@sendArduino(): Value sent in I2C_sendBuf[] = "); printBuffer(I2C_sendBuf);
#endif
}

/*********************** End of I2C subroutines ***********************/

/*
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
}

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
  Serial.print("@processRequestResponse(): Value placed in I2C_recdBuf[] = "); printBuffer(I2C_recdBuf);
#endif
}
*/
