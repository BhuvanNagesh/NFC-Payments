/*
 * ESP8266 MFRC522 UID Reader
 * * This sketch initializes an MFRC522 RFID reader, waits for a
 * PICC (Proximity Integrated Circuit Card, i.e., an NFC/RFID card),
 * and prints its UID to the Serial Monitor.
 * * Hardware Connections (NodeMCU):
 * - MFRC522 VCC  -> 3.3V
 * - MFRC522 GND  -> GND
 * - MFRC522 RST  -> D4 (GPIO 2)
 * - MFRC522 SDA  -> D8 (GPIO 15)
 * - MFRC522 MOSI -> D7 (GPIO 13)
 * - MFRC522 MISO -> D6 (GPIO 12)
 * - MFRC522 SCK  -> D5 (GPIO 14)
 */

#include <SPI.h>        // Include the SPI library
#include <MFRC522.h>    // Include the MFRC522 library

// --- Pin Definitions ---
// Define the pins we used for RST and SS (SDA)
// We use the GPIO numbers directly (e.g., 2, 15) instead of NodeMCU
// aliases (D4, D8) to ensure compatibility with all ESP8266 boards.
#define RST_PIN   2  // GPIO 2 (D4 on NodeMCU)
#define SS_PIN    15  // GPIO 15 (D8 on NodeMCU)

// Create an MFRC522 instance.
MFRC522 mfrc522(SS_PIN, RST_PIN);  

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect (good practice)

  Serial.println("\nInitializing RFID Reader...");

  // Initialize the SPI bus
  SPI.begin();
  
  // Initialize the MFRC522 reader
  mfrc522.PCD_Init();
  
  // A small delay to let the PCD stabilize
  delay(4); 
  
  // Print reader version info to check if connection is OK
  mfrc522.PCD_DumpVersionToSerial();
  Serial.println("Scan your card to read its UID...");
}

void loop() {
  // 1. Look for new cards
  // mfrc522.PICC_IsNewCardPresent() checks for a new card.
  // If a card is not found, the loop repeats, efficiently polling.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return; // No card present, exit loop and try again
  }

  // 2. Select one of the cards
  // mfrc522.PICC_ReadCardSerial() attempts to read the card's UID.
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return; // Failed to read the card, exit loop and try again
  }

  // 3. If we are here, a card was successfully read.
  Serial.print("Card UID: ");
  printUidToSerial();
  Serial.println();

  // 4. Halt the card
  // This is important! It stops the card from being read
  // over and over again, allowing for a new card (or the same one)
  // to be detected as "new" next time.
  mfrc522.PICC_HaltA();

  // 5. Stop encryption (good practice)
  mfrc522.PCD_StopCrypto1();

  // Add a small delay to prevent spamming the serial monitor
  // and give you time to pull the card away.
  delay(1000); 
}

/**
 * Helper function to print the UID byte array as a
 * formatted Hexadecimal string.
 */
void printUidToSerial() {
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    // Add leading '0' if the byte is less than 0x10 (16)
    if (mfrc522.uid.uidByte[i] < 0x10) {
      Serial.print(" 0");
    } else {
      Serial.print(" ");
    }
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    
    // You can also build a string if you need it for processing
    // uidString.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    // uidString.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  // To use the UID string for logic:
  // uidString.toUpperCase();
  // if (uidString.indexOf("AA BB CC DD") > -1) { ... }
}