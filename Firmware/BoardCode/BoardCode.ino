#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "VescUart.h"
VescUart UART;

#define power_relay 3

/////////////////
//Radio Setup
/////////////////
RF24 radio(7, 8); // CE, CSN
const byte addresses[][6] = {"00001", "00002"};

//Struct to send back board telemtery data
struct TelemetryStruct {
  float voltage;
  long rpm;
  long tach_abs;
};
TelemetryStruct telemetry;

//Holds recieved value from controller
byte speedVal = 0;

//Timeout variables for safety
int timeout_duration = 100;
long last_recieve = millis();
int recieve_switchoff_duration = 10000;
int inactive_switchoff_duration = 120000;

//Timer variables for periodically requesting data
int data_request_duration = 500;
long last_request = millis();

void setup() {
  //Radio setup
  radio.begin();
  radio.openWritingPipe(addresses[0]); // 00001
  radio.openReadingPipe(1, addresses[1]); // 00002
  radio.setPALevel(RF24_PA_LOW);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();

  //Initialize telemetry variables
  telemetry.voltage = 6.9;
  telemetry.rpm = 420;
  telemetry.tach_abs = 69420;

  //Set up ack payload
  radio.writeAckPayload(1, &telemetry, sizeof(telemetry));

  //Bedin serial communication with VESC
  Serial.begin(115200);
  while (!Serial) {;}
  UART.setSerialPort(&Serial);

  pinMode(power_relay, OUTPUT);
  digitalWrite(power_relay, LOW);
  delay(250);
  digitalWrite(power_relay, HIGH);
}

void loop() {
  //Listen for a command from the controller
  radio.startListening();
  if ( radio.available()) {
    while (radio.available()) {
      
      //Recieve controller value
      byte angleV = 0;
      radio.read(&speedVal, sizeof(speedVal));
      //Serial.println(speedVal);
      
      //Send back board telemetry
      telemetry.voltage = UART.data.inpVoltage;
      telemetry.rpm = UART.data.rpm;
      telemetry.tach_abs = UART.data.tachometerAbs;
      radio.writeAckPayload(1, &telemetry, sizeof(telemetry));

      //Remember when recieved last message
      last_recieve = millis();
    }
  }

  //If recently recieved a command, send it to the board 
  if (millis()-last_recieve < timeout_duration){
    UART.nunchuck.valueY = speedVal;
    UART.setNunchuckValues();
  }
  //If haven't recieved a message in a long time, stop the board
  else{
    UART.nunchuck.valueY = 128;
    UART.setNunchuckValues();
  }

  //If haven't recieved a message in a very long time, turn off the board
  if (millis()-last_recieve > recieve_switchoff_duration){
    digitalWrite(3,LOW);
  }

  //Ask vesc for telemetry every once in a while
  if (millis()-last_request > data_request_duration){
    UART.getVescValues();
    last_request = millis();
  }
}
