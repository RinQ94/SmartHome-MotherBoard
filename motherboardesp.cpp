//----------------------------------------------------------------------------------------
// Code for ESP32 Smart Home System
// Created by RinQ
// Code for motherboard, connected to 8 relays and 6 buttons
// and another ESP32 for temmperature sensors
// ToDo: 1. OTA V
//       2. Connect to slave - panel in bathroom (LED, buttons, temperature sensor)
//       3. Connect to slave - panel in den (LED, buttons, temperature sensor)
//       4. State Machine V
//----------------------------------------------------------------------------------------
#include "OneButton.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>

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

#define POWER_ON_PIN 23
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

// Table for room names
const char* rooms[] = { "kuchnia", "biuro", "lazienka", "salon", "balkon", "szafka_elektryczna" };

WebServer server(80);

// Wi-Fi
const char* ssid = "...";
const char* password = "...";

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
  pinMode(POWER_ON_PIN, OUTPUT);
  digitalWrite(POWER_ON_PIN, LOW); // Power supply on
  delay(200); // Delay for power suply

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

  // OTA, WIFI
  setupWiFi();
  setupOTA();
  setupServer();
}

void loop() {

  server.handleClient();
  ArduinoOTA.handle();
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

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z siecią Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nPołączono z Wi-Fi.");
  Serial.print("Adres IP: ");
  Serial.println(WiFi.localIP());
}

void setupOTA() {
  ArduinoOTA.begin();
  Serial.println("OTA ready.");
}

void setupServer() {
  server.on("/lights", handleWebPage); // mainpage
  server.on("/getRelayStates", handleGetRelayStates);
  server.on("/setRelay", handleSetRelay);
  server.begin();
  Serial.println("HTTP Running.");
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
  relayStates = 0x00;
}

void long2() {
  Serial.println("Long 2 clicked.");
  if (hiddenStates != 0x00) relayStates = hiddenStates;
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

void handleGetRelayStates() {
  server.send(200, "text/plain", String(relayStates));
}

void handleSetRelay() {
  if (server.hasArg("id") && server.hasArg("state")) {
      String relayIdStr = server.arg("id");
      String stateStr = server.arg("state");

      uint8_t relayIndex = static_cast<uint8_t>(relayIdStr.substring(5).toInt() - 1);
      if (relayIndex >= 0 && relayIndex < 8) {
        if (stateStr == "on") {
          relayStates |= (1 << relayIndex);
          } else {
        relayStates &= ~(1 << relayIndex);
      }

      updateRelays();      
    }
  }
}

void handleWebPage() {
  const String htmlPart1 = R"rawliteral(
  <!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>ESP32 Panel Sterowania (Dynamiczny)</title>
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200" />

    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #1a1a1a;
            color: #e0e0e0;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
        }
        h1 {
            text-align: center;
            color: #f0f0f0;
            margin-bottom: 30px;
            font-weight: 300;
        }
        .container {
            width: 90%;
            max-width: 900px;
            background: #282828;
            padding: 25px;
            border-radius: 12px;
            box-shadow: 0 8px 16px rgba(0, 0, 0, 0.4);

            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 20px;
            justify-content: center;
        }


        .relay-item {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            padding: 20px 10px;
            background-color: #333;
            border-radius: 10px;
            cursor: pointer;
            transition: background-color 0.2s ease-in-out, transform 0.1s ease-in-out;
            text-align: center;
        }
        .relay-item:hover {
            background-color: #444;
            transform: translateY(-3px);
        }
        .relay-item:active {
            transform: scale(0.98);
        }

        .relay-name {
            font-weight: 400;
            font-size: 0.9em;
            color: #cccccc;
            margin-top: 10px;
        }


        .icon-display {
            background: none;
            border: none;
            font-size: 3em;
            padding: 0;
            line-height: 1;
            color: #777;
            transition: color 0.3s ease-in-out, text-shadow 0.3s ease-in-out;
            font-variation-settings: 'FILL' 0, 'wght' 400;
        }

        .relay-item.active .icon-display {
            color: #FFA500;
            text-shadow: 0 0 8px #FFA500, 0 0 15px #FFA500;
            font-variation-settings: 'FILL' 1, 'wght' 600;
        }

        .relay-item.active {
            background-color: #4CAF50;
            /* background-color: #FF8C00; */ /* Darker Orange */
        }

        .relay-item.active .icon-display:hover {
            color: #FFC04D; 
            text-shadow: 0 0 10px #FFC04D, 0 0 20px #FFC04D;
        }

        @media (max-width: 600px) {
            .container {
                grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
                gap: 15px;
                padding: 15px;
            }
            .icon-display {
                font-size: 2.5em;
            }
            .relay-name {
                font-size: 0.8em;
            }
        }
        @media (max-width: 400px) {
            .container {
                grid-template-columns: repeat(auto-fit, minmax(90px, 1fr));
                gap: 10px;
            }
        }
    </style>
</head>
<body>
    <h1>ESP32 Panel Sterowania</h1>
    <div class='container' id="relayContainer">
        </div>

    <script>
        const relays = [)rawliteral";

const String htmlPart3 = R"rawliteral(
        ];

        const relayContainer = document.getElementById('relayContainer');

        function renderRelays() {
            relayContainer.innerHTML = '';
            relays.forEach(relay => {
                const relayItem = document.createElement('div');
                relayItem.classList.add('relay-item');
                relayItem.setAttribute('data-relay-id', relay.id);

                const iconElement = document.createElement('span');
                iconElement.classList.add('material-symbols-outlined', 'icon-display');
                iconElement.textContent = relay.iconName;

                const relayName = document.createElement('span');
                relayName.classList.add('relay-name');
                relayName.textContent = relay.name;

                if (relay.state) {
                    relayItem.classList.add('active');
                } else {
                    relayItem.classList.remove('active');
                }

                relayItem.appendChild(iconElement);
                relayItem.appendChild(relayName);

                relayItem.addEventListener('click', function() {
                    const currentRelayId = this.getAttribute('data-relay-id');
                    const targetRelay = relays.find(r => r.id === currentRelayId);

                    if (targetRelay) {
                        targetRelay.state = !targetRelay.state; 
                        this.classList.toggle('active', targetRelay.state); 

                        sendRelayState(currentRelayId, targetRelay.state);
                    }
                });

                relayContainer.appendChild(relayItem);
            });
        }

        function sendRelayState(relayId, state) {
            console.log(`Sending command to ESP32: Relay ${relayId} is now ${state ? 'ON' : 'OFF'}`);
            fetch(`/setRelay?id=${relayId}&state=${state ? 'on' : 'off'}`)
                .then(response => {
                    if (!response.ok) {
                        console.error('Network response was not ok');
                    }
                    return response.text();
                })
                .then(data => console.log('Response from ESP32 (HTTP setRelay):', data))
                .catch(error => console.error('Error sending relay state:', error));
        }

        function fetchAndRenderRelayStates() {
            fetch('/getRelayStates') 
                .then(response => response.text())
                .then(data => {
                    const espRelayStates = parseInt(data);
                    if (!isNaN(espRelayStates)) {
                        relays.forEach((relay, index) => {
                            const isBitSet = (espRelayStates >> index) & 1;
                            relay.state = (isBitSet === 1);
                        });
                        renderRelays();
                    } else {
                        console.error("Received invalid data from /getRelayStates:", data);
                    }
                })
                .catch(error => console.error('Error fetching relay states:', error));
        }

        document.addEventListener('DOMContentLoaded', () => {
            renderRelays();
            fetchAndRenderRelayStates();
            setInterval(fetchAndRenderRelayStates, 2000);
        });
    </script>
</body>
</html>
)rawliteral";

    String htmlPart2 = "";

    for (uint8_t i = 0; i < 8; i++) {
        String relayId = "relay" + String(i + 1);
        String relayName;
        String iconName;
        bool currentState = (relayStates & (1 << i));

        switch (i){
        case 0: relayName = "Salon"; iconName = "tv_gen"; break;
        case 1: relayName = "Kuchnia"; iconName = "microwave"; break;
        case 2: relayName = "Przedpokój"; iconName = "door_front"; break;
        case 3: relayName = "Łazienka"; iconName = "bathtub"; break;
        case 4: relayName = "Biuro"; iconName = "work"; break;
        case 5: relayName = "Kuchnia 12V"; iconName = "lightbulb"; break;
        case 6: relayName = "Salon 2"; iconName = "light"; break;
        case 7: relayName = "Balkon"; iconName = "balcony"; break;
        } 
        htmlPart2 += "{ id: '" + relayId + "', name: '" + relayName + "', state: " + (currentState ? "true" : "false") + ", iconName: '" + iconName + "' }";
        if (i < 7) {
            htmlPart2 += ",\n";
        } else {
            htmlPart2 += "\n";
        }
        
//            { id: 'relay1', name: 'Salon', state: false, iconName: 'tv_gen' },
    }
    
        String FullHTML = htmlPart1 + htmlPart2 + htmlPart3;
        server.send(200, "text/html", FullHTML);
}
