/*
 * =================================================================
 * FINAL ESP8266 NFC Payment Terminal Code (v4 - Transaction ID FIX)
 * =================================================================
 * This code is matched to your project folder named "Start".
 *
 * It now passes a unique Transaction ID (tid) from the frontend
 * to the backend to prevent race conditions and fix the
 * "stuck page" bug.
 *
 * --- HARDWARE CONNECTIONS (NodeMCU) ---
 * - MFRC522 VCC  -> 3.3V
 * - MFRC522 GND  -> GND
 * - MFRC522 RST  -> D4 (GPIO 2)
 * - MFRC522 SDA  -> D8 (GPIO 15)  (This is the SS/Chip Select pin)
 * - MFRC522 MOSI -> D7 (GPIO 13)
 * - MFRC522 MISO -> D6 (GPIO 12)
 * - MFRC522 SCK  -> D5 (GPIO 14)
 * - Buzzer       -> D0 (GPIO 16)
 */

//----Include Libraries---//
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// --- Pin Definitions ---
#define RST_PIN   2   // GPIO 2 (D4 on NodeMCU)
#define SS_PIN    15  // GPIO 15 (D8 on NodeMCU)
#define LED_PIN     LED_BUILTIN // On-board LED
#define Buzzer_PIN  16  // GPIO 16 (D0 on NodeMCU)

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

//-----SSID and Password for the Access Point-------//
const char* ssid = "Credit Account";
const char* password = "0987654321";

//----Backend Server Settings (from your PHP file)----//
const String apikey = "somade_daniel";
// This points to your "Start" folder
const String servername = "http://192.168.4.2/Start/backend/process_payment.php";

ESP8266WebServer server(80);  // Server on port 80

// --- Global Variables for Transaction State ---
String  currentPaymentType = ""; // Stores "credit" or "debit"
String  currentTransactionID = ""; // <-- NEW: Stores the transaction ID
unsigned long lastScanTime = 0;
const unsigned long scanCooldown = 3000; // 3-second cooldown

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\nStarting NFC Payment Terminal (v4 - TID FIX)...");

  pinMode(LED_PIN, OUTPUT);
  pinMode(Buzzer_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED is off by default (HIGH on built-in)
  digitalWrite(Buzzer_PIN, LOW); // Buzzer off

  // --- Initialize RFID Reader ---
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Checking MFRC522 connection...");
  mfrc522.PCD_DumpVersionToSerial(); // This checks if the reader is connected.

  // --- Create the Access Point ---
  WiFi.softAP(ssid, password);
  Serial.print("Access Point Started: ");
  Serial.println(ssid);
  Serial.print("AP IP address (My IP): ");
  Serial.println(WiFi.softAPIP()); // This should be 192.168.4.1

  // --- Configure Server Handlers ---
  server.on("/setTransaction", handleSetTransaction);

  server.begin(); // Start the server
  Serial.println("HTTP server started. Waiting for transaction type...");

  playSuccessFeedback(); // Beep once to show it's ready
}

/**
 * This function is called by the FRONTEND (e.g. /setTransaction?type=debit&tid=12345678)
 */
void handleSetTransaction() {
  // Send the "permission slip" (CORS Header) to the browser
  server.sendHeader("Access-Control-Allow-Origin", "*");

  if (currentPaymentType != "") {
    Serial.println("Warning: Another transaction is already in progress.");
    server.send(409, "text/plain", "Error: Transaction in progress. Please wait.");
    return;
  }

  // --- MODIFIED: Check for both 'type' and 'tid' ---
  if (server.hasArg("type") && server.hasArg("tid")) {
    currentPaymentType = server.arg("type");
    currentTransactionID = server.arg("tid"); // <-- NEW: Store the ID
    
    Serial.print("Transaction armed! Type: ");
    Serial.print(currentPaymentType);
    Serial.print(", ID: ");
    Serial.println(currentTransactionID);

    // Send a success response back to the frontend
    server.send(200, "text/plain", "Transaction armed. Please scan card.");
  } else {
    Serial.println("Error: /setTransaction call missing 'type' or 'tid'.");
    server.send(400, "text/plain", "Error: 'type' or 'tid' argument missing.");
  }
}

void loop() {
  // Always handle web server requests
  server.handleClient();

  // ----- RFID Scanning Logic -----

  // Add a small delay to prevent the loop from running too fast
  // and interfering with the web server.
  delay(50); 

  if (millis() - lastScanTime < scanCooldown) {
    return; // Still in cooldown from last scan
  }

  if (currentPaymentType == "") {
    return; // Reader is not armed, do nothing
  }

  // Look for new card
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // --- A Card was successfully read AND a transaction is armed ---
  Serial.println("--------------------------");
  Serial.println("Card detected!");
  lastScanTime = millis();

  String uidString = getUidString();
  Serial.print("Card UID: ");
  Serial.println(uidString);

  // --- MODIFIED: Send data to the PHP Backend ---
  // Now includes the transaction ID
  String serverApi = servername + "?apikey=" + apikey + "&paymentType=" + currentPaymentType + "&tid=" + currentTransactionID;
  String request = serverApi + "&card_number=" + uidString;
  
  Serial.print("Sending request to backend: ");
  Serial.println(request);

  WiFiClient client;
  HTTPClient http;

  if (http.begin(client, request.c_str())) {
    http.setTimeout(10000);
    int httpResponseCode = http.GET();
    String payload = "Error: No response";

    if (httpResponseCode > 0) {
      payload = http.getString();
      Serial.print("Backend Response [");
      Serial.print(httpResponseCode);
      Serial.print("]: ");
      Serial.println(payload);

      if (payload.indexOf("successful") > -1 || payload.indexOf("credited") > -1) {
        playSuccessFeedback();
      } else {
        playErrorFeedback();
      }

    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
      playErrorFeedback();
    }
    http.end();
  } else {
    Serial.println("Could not begin HTTP connection to backend.");
    playErrorFeedback();
  }

  Serial.println("Transaction complete. Waiting for next /setTransaction call.");
  Serial.println("--------------------------");
  
  currentPaymentType = ""; // Disarm the reader
  currentTransactionID = ""; // <-- NEW: Clear the ID

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}


/**
 * Helper function to convert the UID byte array to a
 * formatted Hexadecimal string.
 */
String getUidString() {
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidString.concat("0");
    }
    uidString.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  uidString.toUpperCase();
  return uidString;
}

/**
 * Feedback function for success
 */
void playSuccessFeedback() {
  digitalWrite(LED_PIN, LOW); // LED ON
  digitalWrite(Buzzer_PIN, HIGH); // Buzzer ON
  delay(200);
  digitalWrite(LED_PIN, HIGH); // LED OFF
  digitalWrite(Buzzer_PIN, LOW); // Buzzer OFF
}

/**
 * Feedback function for error/timeout
 */
void playErrorFeedback() {
  digitalWrite(LED_PIN, LOW);
  digitalWrite(Buzzer_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(Buzzer_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(Buzzer_PIN, HIGH); 
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(Buzzer_PIN, LOW);
}