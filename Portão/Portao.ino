/*
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2018 Sensnology AB
   Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   REVISION HISTORY
   Version 1.0 - Henrik Ekblad

   DESCRIPTION
   Example sketch showing how to control physical relays.
   This example will remember relay state after power failure.
   http://www.mysensors.org/build/relay
*/

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24

// Enable repeater functionality for this node
//#define MY_REPEATER_FEATURE

#include <MySensors.h>

#define PRIMARY_CHILD_ID 4
#define SECONDARY_CHILD_ID 5
#define PRIMARY_BUTTON_PIN 2   // Arduino Digital I/O pin for "acionamento do trinco"
#define SECONDARY_BUTTON_PIN 3 // Arduino Digital I/O pin for "estado do portão"
#define RELAY_PIN 4            // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 1     // Total number of attached relays
#define RELAY_ON 1             // GPIO value to write to turn on attached relay
#define RELAY_OFF 0            // GPIO value to write to turn off attached relay

MyMessage msg(PRIMARY_CHILD_ID, V_TRIPPED);
MyMessage msg2(SECONDARY_CHILD_ID, V_TRIPPED);

void before()
{
  for (int sensor = 1, pin = RELAY_PIN; sensor <= NUMBER_OF_RELAYS; sensor++, pin++) {
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);
    // Set relay to last known state (using eeprom storage)
    digitalWrite(pin, loadState(sensor) ? RELAY_ON : RELAY_OFF);
  }
}

void setup()
{
  // Setup the buttons
  pinMode(PRIMARY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SECONDARY_BUTTON_PIN, INPUT_PULLUP);
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Portao", "1.0");

  for (int sensor = 1, pin = RELAY_PIN; sensor <= NUMBER_OF_RELAYS; sensor++, pin++) {
    // Register all sensors to gw (they will be created as child devices)
    present(sensor, S_BINARY);
  }
  // Register binary input sensor to sensor_node (they will be created as child devices)
  present(PRIMARY_CHILD_ID, S_DOOR);
  present(SECONDARY_CHILD_ID, S_DOOR);
}

void loop()
{
  uint8_t value;
  static uint8_t sentValue = 2;
  static uint8_t sentValue2 = 2;

  // Short delay to allow buttons to properly settle
  sleep(5);

  value = digitalRead(PRIMARY_BUTTON_PIN);

  if (value != sentValue) {
    // Value has changed from last transmission, send the updated value
    send(msg.set(value == HIGH));
    sentValue = value;
  }

  value = digitalRead(SECONDARY_BUTTON_PIN);

  if (value != sentValue2) {
    // Value has changed from last transmission, send the updated value
    send(msg2.set(value == HIGH));
    sentValue2 = value;
  }

  // Sleep until something happens with the sensor
  sleep(PRIMARY_BUTTON_PIN - 2, CHANGE, SECONDARY_BUTTON_PIN - 2, CHANGE, 0);
}

void receive(const MyMessage &message)
{
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type == V_STATUS) {
    // Change relay state
    digitalWrite(message.sensor - 1 + RELAY_PIN, message.getBool() ? RELAY_ON : RELAY_OFF);
    // Store state in eeprom
    saveState(message.sensor, message.getBool());
    // Write some debug info
    Serial.print("Incoming change for sensor:");
    Serial.print(message.sensor);
    Serial.print(", New status: ");
    Serial.println(message.getBool());
  }
}
