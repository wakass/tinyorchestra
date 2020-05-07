#include <Arduino.h>

#include "tiny_player.h"

// #include <TinyWireS.h> //Transparent coming from ATTinyCore
#include <Wire.h>

const byte SLAVE_ADDR = 100;
const byte NUM_BYTES = 4;

volatile byte data[NUM_BYTES] = { 0, 1, 2, 3 };
volatile byte pulse = 0;
volatile int volume = 120;
volatile byte last_received = 0;

volatile int frameCounter = 0;

volatile int lenCounter = 0;
volatile int volCounter = 0;
volatile int swpCounter = 0;

//internal variablublus
volatile byte int_enable = 0; //internal channel enabled flag.

volatile byte int_duty      = 0;
volatile byte int_len       = 0;
volatile byte int_vol       = 0;
volatile byte int_period    = 0;
volatile byte int_addmode   = 0;
volatile byte int_freq      = 0; //more than one byte. Lectori salute canem ahoi
volatile byte int_trigger   = 0;
volatile byte int_len_enable = 0;



void setup() {
    Wire.begin(SLAVE_ADDR);
    Wire.onRequest(requestISR);
    Wire.onReceive(receiveISR);

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
    TCCR0B = 1 <<WGM02 | 3<<CS00;            // 1/64 prescale
    TIMSK = 1 <<OCIE0A;                      // Enable compare match
    OCR0A = 200;                             // Now at ~22050 KHz

    
    pinMode(4, OUTPUT);
    pinMode(1, OUTPUT);

    // byte data = 0x6b;
    // int_freq = data >> 3; 
    // data = 0x01;
    // int_freq |= (data & 0x7) << 5;
    // OCR0A = (0xFF - (int_freq));
}

void lenTick() {
  if (int_len_enable) {
    int_len--; //if reaches zero turn off channel
    if (int_len == 0)
      int_enable = 0;
  }
};

void volTick() {
  volCounter++;
  if (int_period != 0){
    if (volCounter % int_period == 0){ //Every n period the volume envelope changes
          if (int_addmode == 1)
            int_vol++;
          else
            int_vol--;
          if (int_vol >= 0 && int_vol <= 15)
            volume = int_vol; //set hardware volume
        }
    }
};
void swpTick() {

};

void processTrigger(){
  //For square 1
    // Square 1's frequency is copied to the shadow register.
    // The sweep timer is reloaded.
    // The internal enabled flag is set if either the sweep period or shift are non-zero, cleared otherwise.
    // If the sweep shift is non-zero, frequency calculation and the overflow check are performed immediately.
  int_enable = int_trigger;
  //reset other counters?

}

void processTicks() {   //Assuming ticks come in at 512Hz
  if (frameCounter % 2 == 0) //256Hz
    lenTick();
  if (frameCounter % 4 == 0) //128Hz
    swpTick();
  if (frameCounter % 8 == 0) //64Hz
    volTick();
  
}

void loop() {
  processTicks();

}

//Fire the sequencer, pew pew
void metronomeTick() {
  //We don't really care about overflow since the sequencer runs on divisors
  frameCounter++;
}


void processRegisterCommand(byte reg, byte data){
  switch(reg){
    case NR21: //NR21 FF16 DDLL LLLL Duty, Length load (64-L)
        int_duty = data >> 6;   //duty cycle
        int_len  = data & 0x3F; //lowest 6 bits
      break;
    case NR22: //NR22 FF17 VVVV APPP Starting volume, Envelope add mode, period
        int_vol = data >> 4;
        int_addmode = (data & 8) >> 3;
        int_period  = (data & 7);
      break;
    case NR23: //NR23 FF18 FFFF FFFF Frequency LSB
        int_freq = data >> 3; 
          OCR0A = (0xFF - (int_freq));
      break;
    case NR24: //NR24 FF19 TL-- -FFF Trigger, Length enable, Frequency MSB
        int_trigger = (data & 0x80) >> 7;
        int_len_enable = (data & 0x40) >> 6;
        int_freq |= (data & 0x7) << 5;
          OCR0A = (0xFF - (int_freq));
        if (int_trigger)
          processTrigger();
      break;
  }

}

void receiveISR(byte bytes_received) {
  last_received = bytes_received;
  while (Wire.available()) {
    byte r = Wire.read();
    byte reg   = (r & (0xF << 4)) >> 4; //get high nibble
    byte flag  = r & (0xF); //low nibble

    if (flag == METRONOME_TICK)
      metronomeTick();
    else if (Wire.available())
      processRegisterCommand(reg, Wire.read());
  }
}

void requestISR() {
  Wire.write(int_enable); //general enable flag

  Wire.write(int_duty);
  Wire.write(int_len);
  Wire.write(int_vol);
  Wire.write(int_period);
  Wire.write(int_addmode);
  Wire.write(int_freq); //more than one byte. Lectori salute canem ahoi
  Wire.write(int_trigger);
  Wire.write(int_len_enable);
  
}

ISR (TIMER0_COMPA_vect) {
  pulse = pulse ^ 0xF;
  if (pulse == 0xF && int_enable) { //turn off channel if not enabled
    OCR1A = volume; OCR1B = (volume) ^ 255;
  }
  else {
    OCR1A = 0xFF; OCR1B = 0xFF;
  }
 
}
