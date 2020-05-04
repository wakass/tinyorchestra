#include <Arduino.h>
#include <Wire.h>
#include "../../tiny_player/include/tiny_player.h"
#include <tiny_conductor.h>
#include <tinyStatus.h>

void metronomeTick();
void setupSound();
void initSound();
void incrementFrequency();


byte frequency_counter = 50;

unsigned long timeNow = millis();

void metronomeTick() {
    Wire.beginTransmission(0x0); //send transmission on general/broadcast address
    Wire.write(METRONOME_TICK);
    int r = Wire.endTransmission();
    if (r != 0) Serial.println(r);
    
}
void initSound() {
    byte regs[] = {NR21,NR22,NR23,NR24};
    Wire.beginTransmission(SLAVE_ADDR);
    for (int i =0; i<4; i++){
        byte reg_select = regs[i] << 4;
        byte content=0;
        Wire.write(reg_select);
        Wire.write(content);
    }
    Wire.endTransmission();
}

void setupSound(){
    Wire.beginTransmission(SLAVE_ADDR);
    byte reg_select = NR21 << 4;
    byte nr21 = 0x00; //0 duty      00000001    12.5%
    Wire.write(reg_select);
    Wire.write(nr21);

    reg_select = NR22 << 4;
    byte nr22 = 0x0F << 4 | 0 << 3 | 0; //VVVV APPP Starting volume, Envelope add mode, period
    Wire.write(reg_select);
    Wire.write(nr22);

    reg_select = NR23 << 4;
    byte nr23 = 0x2F;//FFFF FFFF Frequency LSB
    Wire.write(reg_select);
    Wire.write(nr23);

    reg_select = NR24 << 4;
    byte nr24 = 1 << 7 | 0 << 6 | 0 << 2; //TL-- -FFF Trigger, Length enable, Frequency MSB
    Wire.write(reg_select);
    Wire.write(nr24);
    
    Wire.endTransmission();
}

void setFrequency(byte freq) {
    Wire.beginTransmission(SLAVE_ADDR);

    byte reg_select = NR23 << 4;
    byte nr23 = freq;//FFFF FFFF Frequency LSB
    Wire.write(reg_select);
    Wire.write(nr23);

    Wire.endTransmission();
}
int freq = 0;
void incrementFrequency() {
    freq++;
    if (freq > 0xFF)
        freq = 0;
    setFrequency(freq);
    

}


void setup() {
    // TCCR2A = 1 << WGM01; //CTC Mode
    // TCCR2B = 6 << CS20; //256 prescale, ==> 62.5 Khz effective clock
    // OCR2A = 122; //~512Hz
    // TIMSK2 |= 1 << OCIE2A; //Enable interrupt fire on compare match
    
    Serial.begin(9600);
    Wire.begin();
    Serial.print(F("\n\nSerial is Open\n\n"));
    initSound();
    setupSound();
    printTinyStatus();

}


// ISR (TIMER2_COMPA_vect) {
//     sei(); //re-enable interrupts in order to send i2c data.
//     metronomeTick();
// }



void loop() {
    
    incrementFrequency();
    delay(10); //small delay or i2c is too fast for the human ear.
    if (millis() - timeNow >= 250) {                                        // trigger every 750mS
        
        timeNow = millis();                                                 // mark preset time for next trigger
    }
}


