/**
   REVISION HISTORY
   Version 1.0 - Ricardo Lourenço
   DESCRIPTION
   Sketch para controlo de garagem, monitoriza o estado do portão,
   abertura e fecho remotamente, iluminação (Domoticz)
   This node also works as a repeader for other nodes
*/

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#include <MySensors.h>
#include <Bounce2.h>
#include <SPI.h>

#define RELAY1_PIN  6  // Arduino Digital I/O pin number for "relé acionamento abertura fecho portão"
#define RELAY2_PIN  7  // Arduino Digital I/O pin number for "relé acionamento iluminação"
#define RELAY3_PIN  8  // Arduino Digital I/O pin number for "relé acionamento aux"
#define BUTTON1_PIN  4  // Arduino Digital I/O pin number for "botão acionamento abertura fecho portão" 
#define BUTTON2_PIN  5  // Arduino Digital I/O pin number for "micro switch estado do portão"
#define BUTTON3_PIN  3  // Arduino Digital I/O pin number for "botão acionament iluminação"

#define CHILD_ID1 50    // Id of the sensor child "acionamento abertura fecho portão"
#define CHILD_ID2 51    // Id of the sensor child "estado do portão"
#define CHILD_ID3 52    // Id of the sensor child "acionamento iluminação"
#define CHILD_ID4 53    // Id of the sensor child "acionamento aux"

#define RELAY_ON 1
#define RELAY_OFF 0

Bounce debouncerA = Bounce();   //"micro switch estado do portão"
Bounce debouncerB = Bounce();   //"botão acionamento abertura fecho portão"
Bounce debouncerC = Bounce();   //"botão acionament iluminação"

int oldValueA = 0;
int oldValueB = 0;
int oldValueC = 0;
int oldValueD = 0;

bool stateA;            // Acionamento portão
bool stateB;            // Estado portão
bool stateC;            // Estado iluminação
bool stateD;            // Estado Aux

MyMessage msgA(CHILD_ID1, V_LIGHT);
MyMessage msgB(CHILD_ID2, V_STATUS);
MyMessage msgC(CHILD_ID3, V_LIGHT);
MyMessage msgD(CHILD_ID4, V_LIGHT);

void setup()
{
  // Setup the buttons
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);

  // After setting up the button, setup debouncer
  debouncerA.attach(BUTTON1_PIN);
  debouncerA.interval(5);
  debouncerB.attach(BUTTON2_PIN);
  debouncerB.interval(5);
  debouncerC.attach(BUTTON3_PIN);
  debouncerC.interval(5);

  // Make sure relays are off when starting up
  digitalWrite(RELAY1_PIN, RELAY_OFF);
  digitalWrite(RELAY2_PIN, RELAY_OFF);
  digitalWrite(RELAY3_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);

  // Set relay to last known state (using eeprom storage)
  stateA = loadState(CHILD_ID1);
  digitalWrite(RELAY1_PIN, stateA ? RELAY_ON : RELAY_OFF);

  stateB = loadState(CHILD_ID2);

  stateC = loadState(CHILD_ID3);
  digitalWrite(RELAY2_PIN, stateC ? RELAY_ON : RELAY_OFF);

  stateD = loadState(CHILD_ID4);
  digitalWrite(RELAY3_PIN, stateD ? RELAY_ON : RELAY_OFF);
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Controlo garagem", "1.0");
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID1, S_DOOR);
  present(CHILD_ID2, S_BINARY);
  present(CHILD_ID3, S_DOOR);
  present(CHILD_ID4, S_DOOR);
}

void loop()
{
  debouncerA.update();                           // Estado do portão (magnetic Switch portão)
  int valueA = debouncerA.read();
  if (valueA != oldValueA && valueA == 0) {
    send(msgA.set(stateA ? false : true), true); // Send new state and request ack back
  }
  oldValueA = valueA;
  debouncerB.update();                            // botão acionamento abertura fecho portão
  int valueB = debouncerB.read();
  if (valueB != oldValueB && valueB == 0) {
    send(msgB.set(stateB ? false : true), true); // Send new state and request ack back
  }
  oldValueB = valueB;
  debouncerC.update();                            // botão acionamento iluminação
  int valueC = debouncerC.read();
  if (valueC != oldValueC && valueC == 0) {
    send(msgC.set(stateC ? false : true), true); // Send new state and request ack back
  }
  oldValueC = valueC;
}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type == V_LIGHT) {
    switch (message.sensor) {

      case CHILD_ID1:
        stateA = message.getBool();
        digitalWrite(RELAY1_PIN, stateA ? RELAY_ON : RELAY_OFF);
        saveState(CHILD_ID1, stateA);
        break;

      case CHILD_ID3:
        stateB = message.getBool();
        digitalWrite(RELAY2_PIN, stateB ? RELAY_ON : RELAY_OFF);
        saveState(CHILD_ID3, stateB);
        break;

      case CHILD_ID4:
        stateC = message.getBool();
        digitalWrite(RELAY3_PIN, stateC ? RELAY_ON : RELAY_OFF);
        saveState(CHILD_ID4, stateC);
        break;
    }
    // Write some debug info
    Serial.print("Incoming change for sensor:");
    Serial.print(message.sensor);
    Serial.print(", New status: ");
    Serial.println(message.getBool());
  }
}
