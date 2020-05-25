#include <Arduino.h>

#include "tiny_player.h"

// #include <TinyWireS.h> //Transparent coming from ATTinyCore
#include <Wire.h>

const byte SLAVE_ADDR = 100;
const byte NUM_BYTES = 4;

volatile byte data[NUM_BYTES] = { 0, 1, 2, 3 };
volatile int volume = 120;
volatile byte last_received = 0;

volatile uint32_t frameCounter = 1; //Make it 1 instead of 0, so we don't trigger in our processticks-loop.

volatile uint32_t lenCounter = 0;
volatile uint32_t volCounter = 0;
volatile uint32_t swpCounter = 0;

volatile byte sqWavePattern[] = {
  0b00000001,
  0b10000001,
  0b10000111,
  0b01111110
};
volatile byte sqWaveCurrent = sqWavePattern[0];

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
//Pulse channel 1
volatile byte int_swp_period = 0;
volatile byte int_swp_negate = 0;
volatile byte int_swp_shift  = 0;
//Internal to pulse channel 1
volatile byte int_swp_enable = 0;
volatile byte int_swp_shadow_freq = 0;
volatile byte int_swp_timer = 0;
//
volatile byte int_vol_enable = 0; //internal volume envelope enable, enable on trigger




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
    OCR0A = 50;

    
    pinMode(4, OUTPUT);
    pinMode(1, OUTPUT);

    processRegisterCommand(NR10,0b00111101);//-PPP NSSS Sweep period, negate, shift
    
    processRegisterCommand(NR21,0xBF);
    processRegisterCommand(NR22,0x60); //VVVV APPP Starting volume, Envelope add mode, period

    processRegisterCommand(NR23,0x22);
    processRegisterCommand(NR24,0x87);
}

void lenTick() {
  if (int_len_enable) {
    if (--int_len == 0)  //if reaches zero turn off channel
      int_enable = 0;
  }
}
void volTick() {
  if (int_period && int_vol_enable){ 
    volCounter++;
    if (volCounter == int_period){ //Every n period the volume envelope changes
      volCounter = 0;
          if (int_addmode == 1)
            int_vol++;
          else
            int_vol--;
          if (int_vol >= 0 && int_vol <= 15)
            volume = TO_HW_VOLUME(int_vol); //set hardware volume
          else
            int_vol_enable = 0;
        }
    }
}
void swpTick() {
  if (--int_swp_timer == 0) {
    
    int_swp_timer = int_swp_period;
    //When it generates a clock and the sweep's internal enabled flag is set and 
    //the sweep period is not zero, a new frequency is calculated and the overflow check is performed
    if (int_swp_enable && int_swp_period){
      
      //If the new frequency is 2047 or less and the sweep shift is not zero, this new frequency is 
      //written back to the shadow frequency and square 1's frequency in NR13 and NR14, 
      //then frequency calculation and overflow check are run AGAIN immediately using this new value, but this second new frequency is not written back. 
      if (int_swp_shift) {
        byte overflow = 0; 
        byte new_freq = swpShiftAndCheckOverflow(overflow);
        
        if (!overflow) {
          int_swp_shadow_freq = new_freq;
          int_freq = new_freq;
          swpShiftAndCheckOverflow();

          OCR0A = (0xFF - (int_swp_shadow_freq));
          }
      }
    }
  }
}

uint16_t swpGetNewFrequency(byte current_freq) {
  uint16_t shifted = current_freq >> int_swp_shift;
  uint16_t new_freq = current_freq;
  
  if (int_swp_negate)
    new_freq -= shifted; //Bug here when shifted becomes 0 (e.g. bit 7 and 7 shift), thus sticking the frequency at the stuck point.
  else 
    new_freq += shifted;

  return new_freq;
}

byte swpShiftAndCheckOverflow(byte &overflow) {
  overflow = 0;
  uint16_t new_freq = swpGetNewFrequency(int_swp_shadow_freq);
  //Turn off the channel if the frequency "overflows"
  if (new_freq > 0xFF) {
    int_swp_enable = 0;
    int_enable = 0;
    overflow = 1;
    return 0xFF;
  }
  return new_freq;
}

void processTrigger(){
  //For square 1
    // Square 1's frequency is copied to the shadow register.
    int_swp_shadow_freq = int_freq;
    // The sweep timer is reloaded.
    int_swp_timer = int_swp_period;
    // The internal enabled flag is set if either the sweep period or shift are non-zero, cleared otherwise.
    if (int_swp_period || int_swp_shift)
      int_swp_enable = 1;
    else
      int_swp_enable = 0;
    // If the sweep shift is non-zero, frequency calculation and the overflow check are performed immediately.
    if (int_swp_shift) swpShiftAndCheckOverflow();
  
  int_enable = 1;
  //reset other counters?
  int_vol_enable = 1;

}

void processTicks() {   //Assuming ticks come in at 512Hz
  if (frameCounter % 2 == 0) //256Hz
    lenTick();
  if ((frameCounter % 4) == 0) //128Hz
    swpTick();
  if (frameCounter % 8 == 0) //64Hz
    volTick();
  
}
volatile uint32_t testcounter = 0xFF;
void loop() {
  
  testcounter -= 1;
  if(testcounter == 0) {
    processTicks();
    testcounter = 0xFF;
    metronomeTick();
    // OCR0A ^= 54;
  }
}

//Fire the sequencer, pew pew
void metronomeTick() {
  //We don't really care about overflow since the sequencer runs on divisors
  frameCounter++;
}




void processRegisterCommand(byte reg, byte data){
  switch(reg){
    case NR10: //NR10 FF10 -PPP NSSS Sweep period, negate, shift
        int_swp_period = (data >> 4) & 0x7;
        int_swp_negate = (data >> 3) & 0x1;
        int_swp_shift  = data & 0x7;
      break;
    case NR21: //NR21 FF16 DDLL LLLL Duty, Length load (64-L)
        int_duty = data >> 6;   //duty cycle
        int_len  = (data & 0x3F); //lowest 6 bits
        sqWaveCurrent = sqWavePattern[int_duty];
      break;
    case NR22: //NR22 FF17 VVVV APPP Starting volume, Envelope add mode, period
        int_vol = data >> 4;
        volume = TO_HW_VOLUME(int_vol);
        int_addmode = (data & 8) >> 3;
        int_period  = (data & 7);
      break;
    case NR23: //NR23 FF18 FFFF FFFF Frequency LSB
        int_freq = data >> 3; 
          OCR0A = (0xFF - (int_freq)); //The square wave period is the inverse apparently of the frequency tick
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

void receiveISR(int bytes_received) {
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
// volatile byte pulse = 0;
ISR (TIMER0_COMPA_vect) {
  byte pulse;
  // Circular shift our sqWave, output current (before shift) MSB to pulse byte
  asm volatile(
    // "mov __tmp_reg__, %0 \n\t"
    "bst %[wave], 7   \n\t" //Store in T flag.
    "eor %[pulse], %[pulse] \n\t" //Zero the variable
    "bld %[pulse], 0  \n\t" //Set the pulse byte
    "rol %[wave]      \n\t" //Rotate left, with bit 7 falling off the end.
    "bld %[wave], 0   \n\t" //Put back into beginning.
    
    : [wave] "=&r" (sqWaveCurrent), [pulse] "=r" (pulse)  //Output operands
    : "0" (sqWaveCurrent)                                 //input operands
    :                                                     //clobbered registers, empty
  );
  if (pulse && int_enable) { //turn off channel if not enabled
    OCR1A = (volume); OCR1B = (volume) ^ 255;
  }
  else {
    OCR1A = 0xFF; OCR1B = 0xFF;
  }
 
}
