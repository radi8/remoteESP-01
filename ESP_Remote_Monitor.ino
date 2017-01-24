#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//#define FEATURE_Internet_Access

const char* ssid = "Guest";
const char* password = "BeMyGuest";

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
  Serial.begin(115200);
  Serial.println();

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
  
  if (packetSize)
  {
    // receive incoming UDP packets
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);

    // send back a reply, to the IP address and port we got the packet from
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(replyPacket);
    Udp.endPacket();
    cmd = getCmd("01 incomingPacket");
  }
}

uint8_t getCmd(char* incomingPacket)
{
  long cmdNumber;
  char * pEnd;

  cmdNumber = strtol(incomingPacket, &pEnd, 10);
  return cmdNumber;
}

