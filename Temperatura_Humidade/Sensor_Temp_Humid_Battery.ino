/*
   REVISION HISTORY
   Version 1.0 - Ricardo Lourenço
   DESCRIPTION
   Sketch para monitorização de temperatura e humidade, sensor utilizado DHT11/DHT-22.
   Monitorização do estado da bateria
 */



// Enable debug prints
#define MY_DEBUG
#define MY_RADIO_RF24
//#define NODE_ID 10
//#define MY_REPEATER_FEATURE

#include <avr/sleep.h>
#include <SPI.h>
#include <MySensors.h>  
#include <DHT.h>

#define DHT_DATA_PIN 3  // Set this to the pin you connected the DHT's data pin to.
#define SENSOR_TEMP_OFFSET 0  // Set this offset if the sensor has a permanent small offset to the real temperatures.
#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1

// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
static const uint64_t UPDATE_INTERVAL = 300000;

// Force sending an update of the temperature after n sensor reads, so a controller showing the
// timestamp of the last update doesn't show something like 3 hours in the unlikely case, that
// the value didn't change since;
// i.e. the sensor would force sending an update every UPDATE_INTERVAL*FORCE_UPDATE_N_READS [ms]
static const uint8_t FORCE_UPDATE_N_READS = 10;
float lastTemp;
float lastHum;
uint8_t nNoUpdatesTemp;
uint8_t nNoUpdatesHum;
bool metric = true;

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
DHT dht;


// BatteryMonitor
#define VMIN 1.9 // (Min input voltage to regulator according to datasheet or guessing. (?) )
#define VMAX 3.25 // (Known or desired voltage of full batteries. If not, set to Vlim.)
int BATTERY_SENSE_PIN = A0; // select the input pin for the battery sense point
int oldBatteryPcnt = 0;


void presentation()  
{ 
  // Send the sketch version information to the gateway
  sendSketchInfo("TempHumExterior_BatMeter", "1.1");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);

  metric = getControllerConfig().isMetric;
}


void setup()
{
  dht.setup(DHT_DATA_PIN); // set data pin of DHT sensor
  if (UPDATE_INTERVAL <= dht.getMinimumSamplingPeriod()) {
    Serial.println("Warning: UPDATE_INTERVAL is smaller than supported by the sensor!");
  }
  // Sleep for the time of the minimum sampling period to give the sensor time to power up
  // (otherwise, timeout errors might occure for the first reading)
  sleep(dht.getMinimumSamplingPeriod());

  // BatteryMonitor - use the 1.1 V internal reference
  analogReference(INTERNAL);

}


void loop()      
{  
  // Force reading sensor, so it works also after sleep()
  dht.readSensor(true);

  // Get temperature from DHT library
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT!");
  } else if (temperature != lastTemp || nNoUpdatesTemp == FORCE_UPDATE_N_READS) {
    // Only send temperature if it changed since the last measurement or if we didn't send an update for n times
    lastTemp = temperature;

    // apply the offset before converting to something different than Celsius degrees
    temperature += SENSOR_TEMP_OFFSET;

    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    // Reset no updates counter
    nNoUpdatesTemp = 0;
    send(msgTemp.set(temperature, 1));

    #ifdef MY_DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
    #endif
  } else {
    // Increase no update counter if the temperature stayed the same
    nNoUpdatesTemp++;
  }

  // Get humidity from DHT library
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum || nNoUpdatesHum == FORCE_UPDATE_N_READS) {
    // Only send humidity if it changed since the last measurement or if we didn't send an update for n times
    lastHum = humidity;
    // Reset no updates counter
    nNoUpdatesHum = 0;
    send(msgHum.set(humidity, 1));

    #ifdef MY_DEBUG
    Serial.print("H: ");
    Serial.println(humidity);
    #endif
  } else {
    // Increase no update counter if the humidity stayed the same
    nNoUpdatesHum++;
  }

 // BatteryMonitor - get the battery Voltage
    int sensorValue = analogRead(BATTERY_SENSE_PIN);
    #ifdef MY_DEBUG
      Serial.println(sensorValue);
    #endif

    // 1M, 470K divider across battery and using internal ADC ref of 1.1V
    // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
    // ((1e6+472e3)/472e3)*1.1 = Vmax = 3.43 Volts
    // 3.43/1023 = Volts per bit = 0.00335338072
    float Vbat  = sensorValue * 0.003363075;
    int batteryPcnt = static_cast<int>(((Vbat-VMIN)/(VMAX-VMIN))*100.);  

    #ifdef MY_DEBUG
        Serial.print("Battery Voltage: ");
        Serial.print(Vbat);
        Serial.println(" V");
        Serial.print("Battery percent: ");
        Serial.print(batteryPcnt);
        Serial.println(" %");
    #endif

    if (oldBatteryPcnt != batteryPcnt) {
        // Power up radio after sleep
        sendBatteryLevel(batteryPcnt);
        oldBatteryPcnt = batteryPcnt;
    }

  // Sleep for a while to save energy
  sleep(UPDATE_INTERVAL); 
}
