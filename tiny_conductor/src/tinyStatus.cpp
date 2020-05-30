#include <Arduino.h>
#include <Wire.h>
#include <tiny_conductor.h>
#include <tinyStatus.h>

const byte NUM_BYTES = 11;
byte data[NUM_BYTES] = { 0 };
byte bytesReceived = 0;

void printTinyStatus() {
    Wire.requestFrom(SLAVE_ADDR, (int) NUM_BYTES);                            // request bytes from slave
    bytesReceived = Wire.available();                                   // count how many bytes received
    if (bytesReceived == NUM_BYTES) {                                   // if received correct number of bytes...
        for (byte i=0; i<NUM_BYTES; i++) data[i] = Wire.read();         // read and store each byte
        printData();                                                    // print the received data
    } else {                                                            // if received wrong number of bytes...
        Serial.print("Invalid number of bytes received: ");
        Serial.println(bytesReceived);
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