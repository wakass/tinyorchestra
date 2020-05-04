## i2c sub-protocol

16bit instructions consisting of:

RRRR FFFF   , register select, special flag
xxxx xxxx   , register content

16 possible registers

##Register definitions

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

## Special flags
#define METRONOME_TICK 0xFF

## Metronome ticks
Sent over general i2c address 0x00. All players receive general call. 