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
*/

// Surge - v0.5 Power monitor with dht11 temp/humidty sensor and battery voltage reporting

#include <SPI.h>
#include <MySensor.h>
#include <EmonLib.h>
#include <Vcc.h>

#define ANALOG_INPUT_SENSOR 3  // The digital input you attached your SCT sensor.  (Only 2 and 3 generates interrupt!)
//#define INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID_PWR 1              // Id of the sensor child

EnergyMonitor emon1;
MySensor gw;
MyMessage wattMsg(CHILD_ID_PWR, V_WATT);
MyMessage kwhMsg(CHILD_ID_PWR, V_KWH);
unsigned long SLEEP_TIME = 60000 - 3735; // sleep for 60 seconds (-4 seconds to calculate values)

long wattsum = 0;
double kwh = 0;
double wh = 0;
int minutes = 61;

//Humidity Sensor Code
#include <DHT.h>
#define CHILD_ID_HUM 2
#define CHILD_ID_TEMP 3
#define HUMIDITY_SENSOR_DIGITAL_PIN 3

DHT dht;
float lastTemp;
float lastHum;
boolean metric = true;
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
//End of Humidity Sensor Code

//battery measuring code
//const float VccMin   = 3.0;           // Minimum expected Vcc level, in Volts.
//const float VccMax   = 4.2;           // Maximum expected Vcc level, in Volts.
const float VccCorrection = 1.012;      // 3.362/3;  // Measured Vcc by multimeter divided by reported Vcc
Vcc vcc(VccCorrection);
float oldBatteryVolt = 0.0;
#define CHILD_ID_BAT 4
MyMessage msgBat(CHILD_ID_BAT, V_VOLTAGE);

void setup()
{

  //energy clamp code
  gw.begin(NULL, AUTO, true);
  Serial.begin(115200);
  emon1.current(ANALOG_INPUT_SENSOR, 30);             // Current: input pin, calibration. stock = 30
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Energy Meter SCT013", "0.3");
  // Register this device as power sensor
  gw.present(CHILD_ID_PWR, S_POWER);
  double Irms = emon1.calcIrms(1480);  // initial boot to charge up capacitor (no reading is taken) - testing
  //end of energy clamp code

  //Humidity Sensor Code
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);
  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  metric = gw.getConfig().isMetric;
  //End of Humidity Sensor Code

  // battery sensor code
  gw.present(CHILD_ID_BAT, S_MULTIMETER);
  delay(2000);
  // end of battery sensor code
}

void loop()
{
  gw.process();

  // power used each minute
  if (minutes < 60) {
    double Irms = emon1.calcIrms(1480);  // Calculate Irms only
    if (Irms < 0.3) Irms = 0;
    long watt = Irms * 240.0; // default was 230 but our local voltage is about 240
    wattsum = wattsum + watt;
    minutes++;
    gw.send(wattMsg.set(watt));  // Send watt value to gw

    Serial.print(watt);         // Apparent power
    Serial.print("W, I= ");
    Serial.print(Irms);          // Irms
  }
  // end power used each minute

  // Humidity/Temp Sensor Code

  if (minutes == 15 || minutes == 30 || minutes == 45 || minutes == 60)  { // eventually reduce this to once or twice per hour
    //   delay(dht.getMinimumSamplingPeriod());  //humm?
    float temperature = dht.getTemperature();
    if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
    } else if (temperature != lastTemp) {
      lastTemp = temperature;
      if (!metric) {
        temperature = dht.toFahrenheit(temperature);
      }
      gw.send(msgTemp.set(temperature, 1));
      Serial.print("T: ");
      Serial.println(temperature);
    }

    float humidity = dht.getHumidity();
    if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
    } else if (humidity != lastHum) {
      lastHum = humidity;
      gw.send(msgHum.set(humidity, 1));
      Serial.print("H: ");
      Serial.println(humidity);
    }
  }
  //end of Humidity/Temp Sensor Code

  // hours KW reading
  if (minutes >= 60) {
    wh = wh + wattsum / 60;
    kwh = wh / 1000;
    gw.send(kwhMsg.set(kwh, 4)); // Send kwh value to gw
    wattsum = 0;
    minutes = 0;
    // end of hourly KW reading

    // Batttery voltage check every hour and reported if different to last check
    float batteryVolt = vcc.Read_Volts();
    if (oldBatteryVolt != batteryVolt)
    {
      Serial.print("Battery Voltage =");
      Serial.print(batteryVolt);
      Serial.println("v");
      gw.send(msgBat.set(batteryVolt, 3));
      oldBatteryVolt = batteryVolt;
    }
    // End of battery voltage check
  }
  //sleep until the next minute
  gw.sleep(SLEEP_TIME);
}
