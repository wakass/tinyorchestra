#include <Arduino.h>
#include <Wire.h>

void printData();

const byte SLAVE_ADDR = 100;

const byte NUM_BYTES = 5;

byte data[NUM_BYTES] = { 0 };

byte bytesReceived = 0;
byte frequency_counter = 50;

unsigned long timeNow = millis();

void setup() {
    Serial.begin(9600);
    Wire.begin();
    Serial.print(F("\n\nSerial is Open\n\n"));
}

void loop() {
    if (millis() - timeNow >= 250) {                                        // trigger every 750mS
        Wire.requestFrom(SLAVE_ADDR, NUM_BYTES);                            // request bytes from slave
        bytesReceived = Wire.available();                                   // count how many bytes received
        if (bytesReceived == NUM_BYTES) {                                   // if received correct number of bytes...
            for (byte i=0; i<NUM_BYTES; i++) data[i] = Wire.read();         // read and store each byte
            printData();                                                    // print the received data
        } else {                                                            // if received wrong number of bytes...
            Serial.print(F("\nRequested "));                                // print message with how many bytes received
            Serial.print(NUM_BYTES);
            Serial.print(F(" bytes, but got "));
            Serial.print(bytesReceived);
            Serial.print(F(" bytes\n"));
        }
        Wire.beginTransmission(SLAVE_ADDR);
        Wire.write(frequency_counter);
        Wire.endTransmission();
        frequency_counter++;
        Serial.println(frequency_counter);
        if (frequency_counter == 255) {
          frequency_counter = 50;
        }
        timeNow = millis();                                                 // mark preset time for next trigger
    }
}

void printData() {
    Serial.print(F("\n"));
    for (byte i=0; i<NUM_BYTES; i++) {
          Serial.print(F("Byte["));
          Serial.print(i);
          Serial.print(F("]: "));
          Serial.print(data[i]);
          Serial.print(F("\t"));
    }
    Serial.print(F("\n"));    
}