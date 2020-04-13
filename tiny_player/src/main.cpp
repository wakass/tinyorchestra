#include <Arduino.h>
#include "tinyorchestra.h"
#include <TinyWireS.h>

const byte SLAVE_ADDR = 100;
const byte NUM_BYTES = 4;

volatile byte data[NUM_BYTES] = { 0, 1, 2, 3 };
volatile byte pulse = 0;
volatile int volume = 120;



void setup() {
    TinyWireS.begin(SLAVE_ADDR);
    TinyWireS.onRequest(requestISR);
    TinyWireS.onReceive(receiveISR);

    // Enable 64 MHz PLL and use as source for Timer1
    PLLCSR = 1<<PCKE | 1<<PLLE;
    OSCCAL = 255;
  
    // Set up Timer/Counter1 for PWM output
    TIMSK = 0;                              // Timer interrupts OFF
    TCCR1 = 1<<PWM1A | 2<<COM1A0 | 1<<CS10; // PWM A, clear on match, 1:1 prescale
    GTCCR = 1<<PWM1B | 2<<COM1B0;           // PWM B, clear on match
    OCR1A = 128; OCR1B = 128;               // 50% duty at start

    // Set up Timer/Counter0 for 8kHz interrupt to output the "real" PWM.
    //OCR0A determines frequency
    TCCR0A = 3 <<WGM00;                      // Fast PWM //Top is OCR0A
    TCCR0B = 1 <<WGM02 | 3<<CS00;            // 1/8 prescale
    TIMSK = 1<<OCIE0A;                      // Enable compare match
    OCR0A = 200;                            // Now at ~22050 KHz

    
    pinMode(4, OUTPUT);
    pinMode(1, OUTPUT);
}

void loop() {}


void receiveISR(byte r) {
  if (r == 42){
    volume = 50;
  }
  
}
void requestISR() {
    for (byte i=0; i<NUM_BYTES; i++) {
        TinyWireS.write(data[i]);
        data[i] += 1;
    }
}
ISR (TIMER0_COMPA_vect) {
  pulse = pulse ^ 0xF;
  if (pulse == 0xF) {
    OCR1A = volume; OCR1B = (volume) ^ 255;
  }
  else {
    OCR1A = 0xFF; OCR1B = 0xFF;
  }
  
}
