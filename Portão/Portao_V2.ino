The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2015 Sensnology AB
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   REVISION HISTORY
   Version 1.0 - Henrik Ekblad

   DESCRIPTION
   Example sketch for a "light switch" where you can control light or something
   else from both HA controller and a local physical button
   (connected between digital pin 3 and GND).
   This node also works as a repeader for other nodes
   http://www.mysensors.org/build/relay
*/

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_RFM69

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <MySensors.h>
#include <Bounce2.h>

#define RELAY1_PIN  4  // Arduino Digital I/O pin number for "relay acionamento abertura fecho portão"
#define RELAY2_PIN  5  // Arduino Digital I/O pin number for "relay acionamento do trinco"
#define BUTTON1_PIN  2  // Arduino Digital I/O pin number for "estado do portão" 
#define BUTTON2_PIN  3  // Arduino Digital I/O pin number for "acionamento do trinco" 
#define CHILD_ID1 1   // Id of the sensor child "relay acionamento abertura fecho portão"
#define CHILD_ID2 2   // Id of the sensor child "estado do portão"
#define CHILD_ID2 3   // Id of the sensor child "acionamento trinco"
#define RELAY_ON 1
#define RELAY_OFF 0

Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();
int oldValue1 = 0;
int oldValue2 = 0;
bool state1;              // Acionamento portão
bool state2;              // Estado portão
bool state3;              // Acionamento trinco

MyMessage msg1(CHILD_ID1, V_LIGHT);
MyMessage msg2(CHILD_ID2, V_STATUS);

void setup()
{
  // Setup the button
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON1_PIN, HIGH);
  digitalWrite(BUTTON2_PIN, HIGH);

  // After setting up the button, setup debouncer
  debouncer1.attach(BUTTON1_PIN);
  debouncer1.interval(5);
  debouncer2.attach(BUTTON2_PIN);
  debouncer2.interval(5);

  // Make sure relays are off when starting up
  digitalWrite(RELAY1_PIN, RELAY_OFF);
  digitalWrite(RELAY2_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  // Set relay to last known state (using eeprom storage)
  state1 = loadState(CHILD_ID1);                              // Acionamento abertura fecho portão
  digitalWrite(RELAY1_PIN, state1 ? RELAY_ON : RELAY_OFF);
  state2 = loadState(CHILD_ID2);                              // Estado do portão
  //  digitalWrite(RELAY2_PIN, state2 ? RELAY_ON : RELAY_OFF);
  state3 = loadState(CHILD_ID3);
}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Controlo portão", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID1, S_LIGHT);
  present(CHILD_ID2, S_BINARY);
}

/*
   Example on how to asynchronously check for new messages from gw
*/
void loop()
{
  debouncer1.update();
  debouncer2.update();
  // Get the update value
  int value = debouncer1.read();                  // Estado do portão (magnetic Switch portão)
  if (value != oldValue1 && value == 0) {
    send(msg2.set(state1 ? false : true), true);  // Send new state and request ack back
    Serial.print("Portão aberto/fechado");
  }
  oldValue1 = value;
  value = debouncer2.read();                      // Acionamento do trinco (estado do relé de sinalização)
  if (value != oldValue2 && value == 0) {
    //send(msg2.set(state2 ? false : true), true); // Send new state and request ack back
    digitalWrite(RELAY2_PIN, RELAY_ON);
    //saveState(CHILD_ID2, state1);
    Serial.print("Trinco aberto/fechado");
  }
  oldValue2 = value;
}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.isAck()) {
    Serial.println("This is an ack from gateway");
  }

  if (message.type == V_LIGHT) {
    // Change relay state
    state1 = message.getBool();
    digitalWrite(RELAY1_PIN, state1 ? RELAY_ON : RELAY_OFF);
    // Store state in eeprom
    saveState(CHILD_ID1, state1);
    //    state2 = message.getBool();
    //    digitalWrite(RELAY2_PIN, state2 ? RELAY_ON : RELAY_OFF);
    //    saveState(CHILD_ID2, state1);

    // Write some debug info
    Serial.print("Incoming change for sensor:");
    Serial.print(message.sensor);
    Serial.print(", New status: ");
    Serial.println(message.getBool());
  }
}
