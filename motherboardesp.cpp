//----------------------------------------------------------------------------------------
// Code for ESP32 Smart Home System
// Created by RinQ
// Code for motherboard, connected to 8 relays and 6 buttons
// and another ESP32 for temmperature sensors
// ToDo: 1. OTA
//       2. Connect to slave - panel in bathroom (LED, buttons, temperature sensor)
//       3. Connect to slave - panel in den (LED, buttons, temperature sensor)
//       4. State Machine
//----------------------------------------------------------------------------------------
#include "OneButton.h"

#define BUTTON_PIN_1 32 //Hall Left button
#define BUTTON_PIN_2 33 //Hall Mid button
#define BUTTON_PIN_3 25 //Hall Right button
#define BUTTON_PIN_4 26 //Left button bedroom
#define BUTTON_PIN_5 27 //Right button bedroom

// For the future{
//#define BUTTON_PIN_6 14
//#define BUTTON_PIN_7 13
//#define BUTTON_PIN_8 12
//}




#define READY_PIN 23
//available for buttons: 27,32,33,25,26,27,14,12,13

#define RELAY_1 4
#define RELAY_2 16
#define RELAY_3 17
#define RELAY_4 5
#define RELAY_5 18
#define RELAY_6 19
#define RELAY_7 21
#define RELAY_8 22

uint8_t Relays[8] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4, RELAY_5, RELAY_6, RELAY_7, RELAY_8};

// RelayStates Variable
uint8_t relayStates = 0b00000000;
int IncomingState;
uint8_t hiddenStates = 0xff;

  // Setup button pins:
  OneButton button1(BUTTON_PIN_1 ,INPUT_PULLUP, true);
  OneButton button2(BUTTON_PIN_2 ,INPUT_PULLUP, true);
  OneButton button3(BUTTON_PIN_3 ,INPUT_PULLUP, true);
  OneButton button4(BUTTON_PIN_4 ,INPUT_PULLUP, true);
  OneButton button5(BUTTON_PIN_5 ,INPUT_PULLUP, true);


void setup() {
  Serial.begin(115200);

  // Initialize button pins
  button1.attachClick(click1);
  button2.attachClick(click2);
  button3.attachClick(click3);
  button4.attachClick(click4);
  button5.attachClick(click5);
  button1.attachDoubleClick(dbclick1);
  button2.attachDoubleClick(dbclick2);
  button3.attachDoubleClick(dbclick3);
  button1.attachLongPressStart(long1);
  button2.attachLongPressStart(long2);

  // Initialize relay pins  
  for (uint8_t i = 0; i < 8; i++){
    pinMode(Relays[i],OUTPUT);
  }
}

void loop() {

  maketicks();
  ReadSerial();
  updateRelays();

  delay(20); // Debounce delay
}

// ticks
void maketicks(){
  button1.tick();
  button2.tick();
  button3.tick();
  button4.tick();
  button5.tick();
}


// click functions
void click1() {
  Serial.println("Btn1_clicked.");
  relayStates ^= (1 << 0);
}

void click2() {
  Serial.println("Btn2_clicked.");  
  relayStates ^= (1 << 1);
}

void click3() {
  Serial.println("Btn3_clicked.");
  relayStates ^= (1 << 2);
}

void click4() {
  Serial.println("Btn4_clicked.");
  relayStates ^= (1 << 3);
}

void click5() {
  Serial.println("Btn5_clicked.");
  relayStates ^= (1 << 4);
}

void dbclick1() {
  Serial.println("Btn1_dbclicked.");
  relayStates ^= (1 << 5);
}

void dbclick2() {
  Serial.println("Btn2_dbclicked.");  
  relayStates ^= (1 << 6);
}

void dbclick3() {
  Serial.println("Btn3_dbclicked.");
  relayStates ^= (1 << 7);
}

void long1() {
  Serial.println("Long 1 clicked.");
  hiddenStates = relayStates;
  relayStates = 0xff;
}


void long2() {
  Serial.println("Long 2 clicked.");
  if (hiddenStates != 0xff) relayStates = hiddenStates;
}

void updateRelays() {
  for (uint8_t i = 0; i < 8; i++){
    digitalWrite(Relays[i], (relayStates & (1 << i)) ? HIGH : LOW );
  }
}

void ReadSerial(){
   if (Serial.available() > 0) {
    IncomingState = Serial.read();
    if ((IncomingState >=0x00) && (IncomingState <= 0xff)){
      relayStates = static_cast<uint8_t>(IncomingState);
    } 
 }
}