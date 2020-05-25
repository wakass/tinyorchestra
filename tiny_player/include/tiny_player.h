void requestISR();
void receiveISR(int bytes_received);

void processRegisterCommand(byte reg, byte data);

byte swpShiftAndCheckOverflow(byte &overflow);
byte swpShiftAndCheckOverflow() {byte dummy; return swpShiftAndCheckOverflow(dummy);}
uint16_t swpGetNewFrequency(byte current_freq);
void swpTick();
void metronomeTick();

//convert the 4bit gameboy volume levels to 8bit PWM we are using (127 is silent, 0 full duty cycle)
//Half cycle is exacty
#define TO_HW_VOLUME(x) (0x7F - ((x << 3) + 7))

//Square wave 1
#define NR10 0
#define NR11 1
#define NR12 2
#define NR13 3
#define NR14 4

//Square wave, pulse channel 2
#define NR20 5 //not used
#define NR21 6
#define NR22 7
#define NR23 8
#define NR24 9

#define METRONOME_TICK 0xF