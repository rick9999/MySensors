/**
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

   EnergyMeterSCT by Patrick Schaerer
   This Sketch is a WattMeter used with a SCT-013-030 non invasive PowerMeter
   see documentation for schematic

   Special thanks to Surge, who optimized my code.

   updated to mySensors Library 2.0
*/


#define MY_RADIO_NRF24
#define MY_REPEATER_FEATURE
#define MY_DEBUG


#include <SPI.h>
#include <MySensors.h>
#include <EmonLib.h>

#define ANALOG_INPUT_CURRENT_SENSOR 6  // The analogic input you attached your current sensor.  (Only 2 and 3 generates interrupt!)
#define ANALOG_INPUT_VOLTAGE_SENSOR 5  // The analogic input you attached your voltage sensor.
//#define INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 1              // Id of the sensor child

EnergyMonitor emon1;

MyMessage wattMsg(CHILD_ID, V_WATT);
MyMessage kwhMsg(CHILD_ID, V_KWH);
MyMessage msgKWH(CHILD_ID, V_VAR1);
unsigned long SLEEP_TIME = 60000 - 3735; // sleep for 60 seconds (-4 seconds to calculate values)

float wattsumme = 0;
float kwh = 0;
float wh = 0;
int minuten = 0;  //vorher 61
boolean KWH_received = false;

//Humidity Sensor Code

#include <DHT.h>
#define CHILD_ID_HUM 2
#define CHILD_ID_TEMP 3
#define HUMIDITY_SENSOR_DIGITAL_PIN 2
DHT dht;
float lastTemp;
float lastHum;
boolean metric = true;
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

//End of Humidity Sensor Code


void setup()
{
  //energy clamp code

  Serial.begin(115200);
  emon1.current(ANALOG_INPUT_CURRENT_SENSOR, 28.51);             // Current: input pin, calibration.
  emon1.voltage(ANALOG_INPUT_VOLTAGE_SENSOR, 295.6, 1.7);        // Voltage: input pin, calibration, phase_shift

  double Irms = emon1.calcIrms(1480);  // initial boot to charge up capacitor (no reading is taken) - testing
  double Vrms = emon.calcVrms;
  request(CHILD_ID, V_VAR1);
  //end of energy clamp code

  //Humidity Sensor Code
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);
  metric = getControllerConfig().isMetric;
  //End of Humidity Sensor Code
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  // Register this device as power sensor
  sendSketchInfo("Energy Meter SCT013", "2.0");
  present(CHILD_ID, S_POWER);
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);

}
void loop()
{

  //process();

  //KWH reveived check
  if (!KWH_received) request(CHILD_ID, V_VAR1);

  // power used each minute
  if (minuten < 60) {
    double Irms = emon1.calcIrms(1480);  // Calculate Irms only
    if (Irms < 0.3) Irms = 0;
    long watt = Irms * 240.0; // default was 230 but our local voltage is about 240
    wattsumme = wattsumme + watt;
    minuten++;
    send(wattMsg.set(watt));  // Send watt value to gw

    Serial.print(watt);         // Apparent power
    Serial.print("W I= ");
    Serial.println(Irms);          // Irms
  }
  // end power used each minute

  // hours KW reading
  if (minuten >= 60) {
    wh = wh + wattsumme / 60;
    kwh = wh / 1000;
    send(kwhMsg.set(kwh, 3)); // Send kwh value to gw
    send(msgKWH.set(kwh, 3)); // Send kwh value to gw
    wattsumme = 0;
    minuten = 0;
  }
  // end of hourly KW reading

  // Humidity Sensor Code
  if (minuten == 15 || minuten == 30 || minuten == 45 || minuten == 60) {
    float temperature = dht.getTemperature();
    if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
    } else if (temperature != lastTemp) {
      lastTemp = temperature;
      if (!metric) {
        temperature = dht.toFahrenheit(temperature);
      }
      send(msgTemp.set(temperature, 1));
      Serial.print("T: ");
      Serial.println(temperature);
    }

    float humidity = dht.getHumidity();
    if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
    } else if (humidity != lastHum) {
      lastHum = humidity;
      send(msgHum.set(humidity, 1));
      Serial.print("H: ");
      Serial.println(humidity);
    }
  }

  //End of Humidity Sensor Code
  wait(SLEEP_TIME);
}

void receive(const MyMessage &message) {
  if (message.type == V_VAR1) {
    kwh = message.getFloat();
    wh = kwh * 1000;
    Serial.print("Received last KWH from gw:");
    Serial.println(kwh);
    //send(kwhMsg.set(kwh, 3)); // Send kwh value to gw
    KWH_received = true;
  }
}
