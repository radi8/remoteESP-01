# remoteESP-01 [![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

*Provide a wifi connection to an Arduino nano via I2C*

<br>

ESP-01 configuration for remote monitoring and switching with an I2C connected Arduino. The device is controlled by a TCP/IP server connection to a remote desktop client which sends commands to this program when buttons are clicked. The remoteESP-01 acting now as master forwards the command via I2C to an arduino slave (remoteArduino) which switches relays, reads voltages or other controls as per the command received and responds with the state of the controlled output which is relayed back to the remote client.
