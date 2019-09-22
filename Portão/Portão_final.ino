/**
   REVISION HISTORY
   Version 1.0 - Ricardo Lourenço
   DESCRIPTION
   Sketch para controlo de portão, monitoriza o estado do portão,
   trinco automático, abertura e fecho remotamente (Domoticz) 
   This node also works as a repeader for other nodes
   Pino2 - estado do portão (Magnetic Switch)
   Pino3 - acionamento do trinco (relé de sinalização do portão)
   Pino4 - relé acionamento abertura fecho portão
   Pino5 - relé acionamento do trinco
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
#define RELAY_ON 1
#define RELAY_OFF 0

Bounce debouncer1 = Bounce();
long time = 0;          // the last time the output pin was toggled
long debounce = 200;    // the debounce time, increase if the output flickers
long debounce1 = 20000; // tempo para trinco on
int oldValue1 = -1;
int state = LOW;        // Current state of the trinco
bool state1;            // Acionamento portão
bool state2;            // Estado portão

MyMessage msg1(CHILD_ID1, V_LIGHT);
MyMessage msg2(CHILD_ID2, V_TRIPPED);

void setup()
{
  // Setup the buttons
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON1_PIN, HIGH);
  digitalWrite(BUTTON2_PIN, HIGH);

  // After setting up the button, setup debouncer
  debouncer1.attach(BUTTON1_PIN);
  debouncer1.interval(5);

  // Make sure relays are off when starting up
  digitalWrite(RELAY1_PIN, RELAY_OFF);
  digitalWrite(RELAY2_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // Set relay to last known state (using eeprom storage)
  state1 = loadState(CHILD_ID1);                              // Acionamento abertura fecho portão
  digitalWrite(RELAY1_PIN, state1 ? RELAY_ON : RELAY_OFF);
}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Controlo portão", "1.0");                  
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID1, S_LIGHT);                                
  present(CHILD_ID2, S_DOOR);
}

void loop()
{
  debouncer1.update();
  int value = debouncer1.read();              // Estado do portão (magnetic Switch portão)
  if (value != oldValue1) {
    send(msg2.set(value == HIGH ? 1 : 0));    // Send new state and request ack back
    Serial.print("Magnetic Switch portão");
  }
  oldValue1 = value;
  int reading = digitalRead(BUTTON2_PIN);     // Acionamento do trinco (estado do relé de sinalização)
  if (reading == LOW && state == LOW && millis() - time > debounce) {
    state = HIGH;
    Serial.print(" Estado do trinco - ");
    Serial.print(state);
    time = millis();
  }
  if (reading == HIGH && state == HIGH && millis() - time > debounce1) {
    state = LOW;
    Serial.print(" Estado do trinco - ");
    Serial.print(state);
    time = millis();
  }
  digitalWrite(RELAY2_PIN, state);
}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.isAck()) {
    Serial.println("This is an ack from gateway");
  }
  if (message.type == V_LIGHT ) {
    // Change relay state
    state1 = message.getBool();
    digitalWrite(RELAY1_PIN, state1 ? RELAY_ON : RELAY_OFF);
    // Store state in eeprom
    saveState(CHILD_ID1, state1);
    // Write some debug info
    Serial.print("Incoming change for sensor:");
    Serial.print(message.sensor);
    Serial.print(", New status: ");
    Serial.println(message.getBool());
  }
}
