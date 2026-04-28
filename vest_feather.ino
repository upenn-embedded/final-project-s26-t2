/*
 * FriendlyFire — Vest FeatherS2 (Client Bridge)
 *
 * Connects to hub Feather's "FriendlyFire" Wi-Fi AP
 * Reads UART from vest ATmega328PB
 * Forwards vest data to hub via WebSocket
 * Receives RESET from hub and sends to vest ATmega via UART TX
 *
 * Board: FeatherS2 (ESP32-S2)
 * Libraries: WebSockets by Markus Sattler
 *
 * Wiring:
 *   Vest ATmega TX (PD1) --[1kOhm]--+-- FeatherS2 RX pin
 *                                    |
 *                                  [2kOhm]
 *                                    |
 *                                   GND
 *   FeatherS2 TX pin ----[1kOhm]------ Vest ATmega RX (PD0)
 *   Vest ATmega GND ------------------- FeatherS2 GND
 *
 * NOTE: Check your FeatherS2 board for the actual GPIO numbers
 *       of the pins labeled RX and TX. Update the defines below.
 *       Common values: RX=44, TX=43  or  RX=38, TX=39
 *       Look at the back of your board or check the pinout diagram.
 */

#include <WiFi.h>
#include <WebSocketsClient.h>

// ===== Pin config — UPDATE THESE for your board =====
#define VEST_RX_PIN  44   // Pin labeled "RX" on your FeatherS2
#define VEST_TX_PIN  43   // Pin labeled "TX" on your FeatherS2
#define RESET_OUT_PIN  5   // drives ATmega PD4 HIGH to reset

// ===== Hub Wi-Fi credentials =====
const char* hubSSID     = "FriendlyFire";
const char* hubPassword = "lasertag1";

// ===== Hub WebSocket =====
WebSocketsClient webSocket;
const char* hubIP   = "192.168.4.1";
const int   hubPort = 81;

// ===== UART buffer =====
String uartBuffer = "";

// ===== WebSocket event handler =====
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected from hub");
      break;

    case WStype_CONNECTED:
      Serial.println("[WS] Connected to hub!");
      break;

    case WStype_TEXT: {
      String msg = String((char*)payload);
      Serial.println("[WS] From hub: " + msg);

      // Check for RESET command from hub (sent by browser)
      // Check for RESET command from hub (sent by browser)
      if (msg == "VEST_RESET") {
        Serial.println("[CMD] Pulsing reset pin to vest ATmega");
        digitalWrite(RESET_OUT_PIN, HIGH);
        delay(100);
        digitalWrite(RESET_OUT_PIN, LOW);
      }
      break;
    }
  }
}

// ===== Setup =====
void setup() {
  // USB serial for debug
  Serial.begin(115200);
  pinMode(RESET_OUT_PIN, OUTPUT);
  digitalWrite(RESET_OUT_PIN, LOW);
  delay(2000);  // S2 needs a moment for USB CDC
  Serial.println("\n=== FriendlyFire Vest Bridge (FeatherS2) ===");

  // UART from vest ATmega — 9600 baud, 8N2 (matches ATmega config)
  Serial1.begin(9600, SERIAL_8N2, VEST_RX_PIN, VEST_TX_PIN);
  Serial.printf("UART listening on RX pin %d, TX pin %d\n", VEST_RX_PIN, VEST_TX_PIN);

  // Connect to hub's Wi-Fi AP
  Serial.print("Connecting to FriendlyFire AP");
  WiFi.begin(hubSSID, hubPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // Connect WebSocket to hub
  webSocket.begin(hubIP, hubPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(2000);
  Serial.println("WebSocket client started — connecting to hub");
}

// ===== Loop =====
void loop() {
  webSocket.loop();

  // Read UART from vest ATmega
  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '\n') {
      if (uartBuffer.length() > 0) {
        Serial.println("[UART] " + uartBuffer);

        // Parse: P2,EVENT,VALUE
        int firstComma  = uartBuffer.indexOf(',');
        int secondComma = uartBuffer.indexOf(',', firstComma + 1);

        if (firstComma > 0 && secondComma > 0) {
          String player = uartBuffer.substring(0, firstComma);
          String event  = uartBuffer.substring(firstComma + 1, secondComma);
          int value     = uartBuffer.substring(secondComma + 1).toInt();

          // Build JSON and send to hub
          String json = "{\"player\":\"" + player + "\",\"event\":\"" + event + "\",\"lives\":" + String(value) + "}";
          webSocket.sendTXT(json);
          Serial.println("[TX] " + json);
        } else {
          Serial.println("[UART] Bad format: " + uartBuffer);
        }

        uartBuffer = "";
      }
    } else if (c != '\r') {
      uartBuffer += c;
      if (uartBuffer.length() > 48) {
        Serial.println("[UART] Dropping oversized line");
        uartBuffer = "";
      }
    }
  }
}
