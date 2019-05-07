#define MY_RADIO_RF24
//#define MY_RF24_PA_LEVEL RF24_PA_MIN
#define MY_DEBUG

#include <SPI.h>
#include <MySensors.h>  
#include <EmonLib.h>              // Include Emon Library

#define SERIAL_BAUD 115200

#define PIN_ANALOG_I1 A0   // The input you attached SCT1.

#define CHILD_ID_WATT 0    // child-id of sensor
#define CHILD_ID_KWH  1    // child-id of sensor
#define CHILD_ID_VAR1 2    // child-id of sensor
#define CHILD_ID_VAR2 3    // child-id of sensor
#define CHILD_ID_VAR3 4    // child-id of sensor
#define CHILD_ID_VAR4 5    // child-id of sensor
#define CHILD_ID_VAR5 6    // child-id of sensor

#define MAX_CT 1           // Number of CT sensor

EnergyMonitor ct1;             // Create an instance

MyMessage apparentPowerMsg(CHILD_ID_VAR2,V_VAR2);
MyMessage powerFActorMsg(CHILD_ID_VAR3,V_VAR3);
MyMessage supplyVoltageMsg(CHILD_ID_VAR4,V_VAR4);
MyMessage IrmsMsg(CHILD_ID_VAR5,V_VAR5);
MyMessage msgWatt(CHILD_ID_WATT,V_WATT);
MyMessage msgKwh(CHILD_ID_KWH,V_KWH);
MyMessage msgVar(CHILD_ID_VAR1,V_VAR1);

unsigned long lastSend_power = millis();
unsigned long lastSend_c = millis();
unsigned long SEND_FREQUENCY   = 60000;               // 1 mn Minimum time between send (in milliseconds).
unsigned long FREQUENCY_SAMPLE = SEND_FREQUENCY / 12 ; // ( 5 s)

#define MAX_CT 1

double  Irms[MAX_CT];
float   nrj[MAX_CT];
boolean pcReceived[MAX_CT];

int    index = 0;

void setup()
{ 
#ifdef MY_DEBUG
  Serial.begin(SERIAL_BAUD);
  Serial.println("Begin SETUP");
#endif
   //ct1.current(PIN_ANALOG_I1, 111.1);     // Current: input pin, calibration.                                        
  ct1.current(PIN_ANALOG_I1, 60);          // current constant = (100 ÷ 0.050) ÷ 33 Homs = 60
  Irms[0] = ct1.calcIrms(1480);            // initial boot to charge up capacitor (no reading is taken) - testing

  for (int i=0; i<MAX_CT; i++) {
       Irms[i] = nrj[i] = 0;
       pcReceived[i]=false;
  }
#ifdef MY_DEBUG
  Serial.println("SETUP completed");
#endif

}

void presentation() {
  
  sendSketchInfo("Energy Meter", "2.1");

  present(0, S_POWER);
  request(0, V_VAR1);
}

void receive(const MyMessage &message) {
  int SensorID ;

  SensorID = message.sensor ;
  
  if (message.type == V_VAR1) {
    
      nrj[SensorID] =  message.getFloat();
      pcReceived[SensorID] = true;

#ifdef MY_DEBUG
      Serial.print("Received last nrj[");
      Serial.print(SensorID);
      Serial.print("] count from GW:");
      Serial.println(nrj[SensorID]);
#endif
  }
}

void send_data(double intrms, int SensorID) {

#ifdef MY_DEBUG
  Serial.print("Send data for SensorID="); 
  Serial.println(SensorID);
#endif

  double energie = (intrms * 240.0 * SEND_FREQUENCY / 1000) / 3.6E6;
  
  nrj[SensorID] += energie;
  send(msgWatt.setSensor(SensorID).set(intrms * 240.0, 1));
  send(msgKwh.setSensor(SensorID).set(nrj[SensorID], 5));
  send(msgVar.setSensor(SensorID).set(nrj[SensorID], 5));
}

void loop() {
  
  unsigned long now = millis();
  double _Irms[MAX_CT];
  bool sendTime_c = now - lastSend_c > FREQUENCY_SAMPLE;
  
  if (sendTime_c)
  {
    _Irms[0] = ct1.calcIrms(1480) ; if (_Irms[0] < 0.3) _Irms[0] = 0;
   
    Irms[0] = (index * Irms[0] + _Irms[0]) / (index + 1);

    lastSend_c = now;
    index++;

#ifdef MY_DEBUG
    for (int i=0; i<MAX_CT; i++) {
        Serial.print("CT");
        Serial.print(i);  Serial.print(" : ");
        Serial.print(Irms[i]*240.0);  // Apparent power
        Serial.print(" ");
        Serial.println(Irms[i]);      // Irms
    }
#endif
  }

  bool sendTime_power = now - lastSend_power > SEND_FREQUENCY;

  if (sendTime_power)
  {
    for (int i=0; i<MAX_CT; i++) {
    // Envoi ou request Puissance1
       if (pcReceived[i])
        { 
         send_data(Irms[i], i);
        } else {
         request(i, V_VAR1);
         Serial.println("Request VAR1");
        }
    }
    //on reinitialise les compteurs
    lastSend_power = now;
    index = 0;
  }
}
