# ardxp
Framework to connect Arduino to X-Plane via UDP using Wiznet W5500 Ethernet module.

_ardxp.ino_ is an empty template. To use it you will need to figure out the datarefs and commands that your interface needs and add code
to subscribe to datarefs, send new dataref values to X-Plane, or send commands. Or any combination of the above.

If you run the sketch as-is it will start the Ethernet module, attempt to get an IP address on your network with DHCP, and look for the
X-Plane PC IP address and port. You will see this happening in the Arduino serial monitor. Then the code enters the classic Arduino `loop()`
and does nothing. Before doing this you need to attach a Wiznet W5500 module to your Arduino, and change the MAC address in the sketch to
something unique for your network (and again for any other Arduinos you want to use on your network).

There are various Ethernet modules you can use. For testing I am using an Arduino Nano and W5500 lite module. The 3.3V regulator on my Nano
does not supply sufficient current for the W5500 lite module so I added an external 3.3V regulator. If you are new to Arduino and Ethernet then
try some of the provided examples in the Arduino IDE to show that your module is connected and working properly.

Tested hardware:
* W5500 blue board
* W5500 lite (green board) with additional 3.3V regulator
* Keyestudio KS0304 Arduino UNO + W5500

Still to do:
* Arduino UNO W5500 shield

The Keyestudio KS0304 board is an Arduino and W5500 combined on one board. Keyestudio also sells the KS0443, which is an Arduino UNO shield with W5500, which I have not tested. Basically any combination of Arduino and W5500 can be used.

Not recommended:
* ENC28J60

The ENC28J60 is an alternative Ethernet interface board, and is supported by Arduino libraries. I can send commands using it, however I have been unable to successfully subscribe to datarefs.

There is a full (almost complete) example in my Challenger 300 glare shield project here:
https://github.com/andrewerrington/challenger_300_glareshield/blob/main/src/glare_shield_interface.ino

You can see how to subscribe to datarefs, how to parse new incoming values, and how to send commands or new values for datarefs back to the
simulator in response to changes detected by the Arduino (button presses, rotary encoders, potentiometer values, etc.).

**N.B.** Unfortunately _XPLMCommandBegin_ and _XPLMCommandEnd_ are not supported by X-Plane on the UDP interface, therefore any cockpit actions that
involve a button or control "held down" can not be activated by this framework. Please ask Laminar to implement it. If you need this then you
have to write your own plugin, although the Arduino code can be used to trigger events in the plugin.
