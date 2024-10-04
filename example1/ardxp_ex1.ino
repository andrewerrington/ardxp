/*
   ardxp - framework to connect Arduino to X-Plane via UDP

   version 0.1  October 2024
   Derived from cut-down code I developed for Challenger 300 glare shield.

   Copyright (c) October 2024 A M Errington

   This code is to interface switches and indicators connected to Arduino
   I/O pins to X-Plane using UDP directly to the simulator (no plug-in
   required). Tested with Wiznet W5500 Ethernet module. ENC28J60 works for
   sending commands, but doesn't seem to work for subscribing to datarefs.

   Some coding required. You need to know how to interface switches,
   indicators, and other hardware to the Arduino. You also need to know
   which commands and datarefs are relevant to your project.

   Example 1: This example shows how to add a potentiometer on A3 to
   control a dataref value.

*/

//        1         2         3         4         5         6         7
// 34567890123456789012345678901234567890123456789012345678901234567890123456789

// Support for Wiznet W5500 Ethernet module
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// Ethernet MAC address, must be unique for all devices on network.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// This code uses DHCP. Your network should assign an IP address to this unit.
// Alternatively, check the Arduino Ethernet examples to show how to set a
// fixed IP address in this code.

// This code also listens for the X-Plane multicast beacon to get the IP
// address and port of the X-Plane PC. If you can't use the beacon, then set
// the IP address of your X-Plane PC on the network here, set xplane_port to
// the default 49000, or whatever you use, and remove the beacon code.
IPAddress xplane_ip(0,0,0,0); // Overwritten by the beacon address later.
unsigned int xplane_port = 0; // X-Plane listens to us on this port.

IPAddress BECN_ip(239,255,1,1); // UDP multicast address for X-Plane beacon.
unsigned int BECN_port = 49707; // X-Plane beacons on this port (was 49000).

unsigned int localPort = 49094; // local port we'll listen on (can be any port).

// An EthernetUDP instance to let us send and receive packets over UDP.
EthernetUDP udp;

// Buffer for receiving and sending UDP data.
char packetBuffer[512];
int packetSize; // Varible to hold received packet size

// A union to convert between bytes and floats
union u_tag {
  byte bin[4];
  float num;
} u;

// Your variables and libraries here

// Analog input to set the zone 1 brightness. This is the top left
// potentiometer on the Pilot's COCKPIT LIGHTS panel for a practical
// example. You can just hook up a potentiometer and control any other
// dataref you know about.
const int zone_1_pot = A3;
uint16_t pot_value = 0;  // variable to store analog input value
uint16_t last_pot_value = -100;  // previous analog input value




// -----------------------------------------------------------------------------

// X-Plane UDP routines

int sendUDP(uint16_t n_bytes) {
  // Send a UDP packet of length 'n_bytes' to X-Plane
  udp.beginPacket(xplane_ip, xplane_port);
  udp.write(packetBuffer, n_bytes);
  return udp.endPacket();
}

void subscribeDataref(const char* dataref, uint16_t freq_Hz, uint8_t index) {
  // Create and send UDP packet to subscribe to a dataref.
  // First argument is a PSTR()
  // e.g.
  // subscribeDataref(PSTR("sim/cockpit2/switches/panel_brightness_ratio[0]"),4,1);
  // This would subscribe to "sim/cockpit2/switches/panel_brightness_ratio[0]"
  // at a frequency of 4Hz, and an index of 1. Later, the simulator will send
  // an RREF packet with an index of 1, so we know the accompanying float
  // value is for "sim/cockpit2/switches/panel_brightness_ratio[0]".

  // General subscription packet is of the form
  // "RREF\x00aaaabbbbpath\to\dataref"
  // Where aaaa is a 32-bit integer representing the desired reporting frequency
  // and bbbb is a 32-bit integer used as an index to tag incoming data so that
  // we know which dataref it refers to. Here they are limited to 8 bits.
  
  // Clear the packet buffer.
  memset(packetBuffer, 0, sizeof(packetBuffer));
  // Build the request in the packet buffer.
  strcpy_P(packetBuffer, PSTR("RREF"));
  memcpy(packetBuffer + 5, &freq_Hz, sizeof(freq_Hz));
  memcpy(packetBuffer + 9, &index, sizeof(index));
  strcpy_P(packetBuffer + 13, dataref);
  sendUDP(413); // Subscription packet is 413 bytes.
}

void writeDataref(const char* dataref, float val) {
  // Create and send UDP packet to write a float value to a dataref.
  // First argument is a PSTR()
  // e.g.
  // writeDataref(PSTR("sim/cockpit2/switches/panel_brightness_ratio[0]"),0.5);

  // General write dataref packet is of the form
  // "DREF\x00nnnnpath\to\dataref"
  // Where nnnn is a 32-bit float to be written to the dataref.

  // Clear the packet buffer
  memset(packetBuffer, 0, sizeof(packetBuffer));
  // Build the request in the packet buffer.
  strcpy_P(packetBuffer, PSTR("DREF"));
  memcpy(packetBuffer + 5, &val, sizeof(val));
  strcpy_P(packetBuffer + 9, dataref);
  sendUDP(509); // Dataref packet is 509 bytes.
}

void sendCommand(const char* command) {
  // Create and send UDP packet to for an X-Plane command.
  // Argument is a PSTR()
  // e.g.
  // sendCommand(PSTR("sim/annunciator/clear_master_caution"));

  // General command packet is of the form
  // "CMND\x00path\to\command\x00"

  // No need to clear the packet buffer as we will send exactly what we put in.
  // Build the command in the packet buffer. PSTR contains the trailing \x00.
  strcpy_P(packetBuffer, PSTR("CMND"));
  strcpy_P(packetBuffer + 5, command);
  sendUDP(strlen_P(command) + 6); // Command packet is only as long as needed.
}

void setup() {
  // You'll probably need a serial port open for debugging.
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println(F("ardxp example 1 - start"));

  // Set up Ethernet
  // Times out after about 1 minute if no DHCP server responds.
  Serial.println(F("Setting up Ethernet."));
  Serial.print(F("MAC address: "));
  for (byte i = 0; i < 6; ++i) {
    if (mac[i] < 0x0F) Serial.print(F("0"));
    Serial.print(mac[i], HEX);
    if (i < 5)
      Serial.print(F(":"));
  }
  Serial.println();

  Serial.println(F("Waiting for DHCP."));
  Ethernet.begin(mac);
  
  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());

  // If you don't want to listen for a BECN packet you could store your
  // X-Plane PC IP address in EEPROM, or hard-code it.

  // This code will listen for the current BECN and capture the IP address
  // of the X-Plane PC, and the port number it is listening on, but only
  // if the PC role is '1'. See X-Plane UDP documentation for more info.
  // We don't check the reported version number for compatibility. At some
  // point in the future things will break. Sorry.

  Serial.println(F("Listening for multicast beacon."));
  udp.beginMulticast(BECN_ip, BECN_port);

  while (true){
    packetSize = udp.parsePacket();
    if (packetSize){
      Serial.print(F("Received packet of size "));
      Serial.println(packetSize);
      udp.read(packetBuffer, packetSize);

      if (strncmp(packetBuffer, "BECN\x00", 5) == 0) {
        Serial.println(F("Packet is BECN."));
        
        // role is a 32 bit unsigned integer, but only has values of 1, 2,
        // or 3, so we will just look at the low byte. We are looking for
        // the master PC, which has a role of 1.

        if (packetBuffer[15]==1) {
          Serial.println(F("Role is 1."));

          // Keep the IP address of the X-Plane PC
          xplane_ip = udp.remoteIP();
          Serial.print(F("X-Plane IP: "));
          Serial.println(xplane_ip);

          // Keep the port number the X-Plane PC is using
          memcpy(&xplane_port,packetBuffer+19,2);
          Serial.print(F("X-Plane port: "));
          Serial.println(xplane_port);
          
          // We have the address and port, break out.
          // Otherwise loop forever, because we don't know the address of the
          // X-Plane PC.
          break;
        }
      }
    }
    delay(10);
  }

  // start UDP
  udp.begin(localPort);

  // Power-on test
  Serial.println(F("Self test."));
  // Do some self-test code here if needed.

  Serial.println(F("Self test done."));

  // Unsubscribe in case we have been reset.
  // Unsubscribe is the same as subscribing, but with a frequency of 0.
  Serial.println(F("Unsubscribing from datarefs."));
  //subscribeDataref(PSTR("cl300/mast_warn"), 0, 0x01);
  //subscribeDataref(PSTR("cl300/mast_caut"), 0 , 0x02);
  //subscribeDataref(PSTR("cl300/gshldl"), 0, 0x03);
  //subscribeDataref(PSTR("cl300/gshldr"), 0, 0x04);

  // Subscribe to the datarefs you want, setting the frequency you'd like to
  // receive the dataref, and a tag, or index, which you will use later to
  // determine what data you have received.
  Serial.println(F("Subscribing to datarefs."));

  // Subscribe to Master Warning, 20 per second, with an index of 0x01
  //subscribeDataref(PSTR("cl300/mast_warn"), 20, 0x01);

  // Subscribe to Master Caution, 20 per second, with an index of 0x02
  //subscribeDataref(PSTR("cl300/mast_caut"), 20 , 0x02);

  // Subscribe to gshldl, five times per second, with an index of 0x03.
  // This will be Zone1
  //subscribeDataref(PSTR("cl300/gshldl"), 5, 0x03);

  // Subscribe to gshldr, five times per second, with an index of 0x04.
  // This will be Zone3r
  //subscribeDataref(PSTR("cl300/gshldr"), 5, 0x04);

  Serial.println(F("setup() done."));

} // setup()


void loop() {

  // Check for incoming UDP packet
  packetSize = udp.parsePacket();

  if (packetSize) {

    // read the packet into packetBuffer
    udp.read(packetBuffer, packetSize);
    
    // The following code is for debugging only.
    Serial.print(F("Received "));
    Serial.print(packetSize);
    Serial.print(F(" byte packet from "));
    Serial.print(udp.remoteIP());
    Serial.print(F(":"));
    Serial.println(udp.remotePort());
    
    Serial.println(F("Contents:"));
    for (int i = 0; i < packetSize; i++) {
      Serial.print((uint8_t)packetBuffer[i], HEX);
      Serial.print(" ");
    
      Serial.println();

      for (int i = 0; i < packetSize; i++) {
        if (packetBuffer[i] < 32) {
          Serial.print("\\x");
          if (packetBuffer[i] < 16) Serial.print("0");
          Serial.print((uint8_t)packetBuffer[i], HEX);
        }
        else
          Serial.print(packetBuffer[i]);
      }
      Serial.println();
    }
    // End debug information

    // Look at the contents of the packet.
    // We're only interested in RREF packets.
    if (strncmp(packetBuffer, "RREF", 4) == 0) {
      // Start at position 5 and unpack the rest
      for (int i = 5; i < packetSize; i += 8) {
        // Unpack the dataref value. It's always a float, so assemble these
        // four bytes into a float
        u.bin[0] = packetBuffer[i + 4];
        u.bin[1] = packetBuffer[i + 5];
        u.bin[2] = packetBuffer[i + 6];
        u.bin[3] = packetBuffer[i + 7];
        float out = u.num;
        // Check the index value, which will be whatever we used when
        // subscribing. It's a 32-bit value, but we only need the first
        // byte as we will only subscribe to a small number of items.

        switch (packetBuffer[i]) {
          case 1: // Dataref we tagged with index 0x01
            // Do something with 'out' float value
            break;
          case 2: // Dataref we tagged with index 0x02
            // Do something with 'out' float value
            break;
          case 3: // Dataref we tagged with index 0x03
            // Do something with 'out' float value
            break;
          case 4: // Dataref we tagged with index 0x04
            // Do something with 'out' float value
            break;
            // etc.
          default:
            // The packet doesn't contain any indexes we handle.
            break;
        }
      }
    }
  } // if (packetSize)

  // We have handled incoming packets. Now look at your inputs. Buttons,
  // switches, potentiometers, etc. If something has changed and an update
  // must be sent to the simulator then use
  //   sendCommand(PSTR("path/to/command"));
  // or
  //   writeDataref(PSTR("path/to/dataref"), value);
  // to send data to the simulator.

  // Let's look at the analog input
  pot_value = analogRead(zone_1_pot);
  if (abs(last_pot_value-pot_value)>=4){
    Serial.print("Pot raw: ");
    Serial.println(pot_value);
    Serial.print("Pot value changed. Writing to dataref.");
    writeDataref(PSTR("cl300/gshldl_h"),pot_value/1023.0);
    last_pot_value=pot_value;
  }


  delay(10);

} // loop()
