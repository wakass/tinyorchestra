## Inspiration

register-like behaviour of waveform generators

e.g. in gameboy:

```
	       Square 1
	NR10 FF10 -PPP NSSS Sweep period, negate, shift
	NR11 FF11 DDLL LLLL Duty, Length load (64-L)
	NR12 FF12 VVVV APPP Starting volume, Envelope add mode, period
	NR13 FF13 FFFF FFFF Frequency LSB
	NR14 FF14 TL-- -FFF Trigger, Length enable, Frequency MSB
```
A timer generates an output clock every N input clocks --> gb frequency = 4194304 (4.19MHz)
a timer's rate is given as a frequency, its period is 4194304/frequency in Hz

Ts = (2048-frequency)*4 //square wave period.
T1 = N/4194304 			//Timer1 period.
Tfreq = 


Square waves use this clock as input.
Translating to PWM in avr 

###Sequencer
Steps at 512Hz

```
Step   Length Ctr  Vol Env     Sweep
---------------------------------------
0      Clock       -           -
1      -           -           -
2      Clock       -           Clock
3      -           -           -
4      Clock       -           -
5      -           -           -
6      Clock       -           Clock
7      -           Clock       -
---------------------------------------
Rate   256 Hz      64 Hz       128 Hz
```
###Code
//Runs at 512 Hz
volatile byte GBT1Counter;
//Tie ocr0b frequency to gameboy frequency
//Period F_CPU
SequencerISR{
	counterLength++;
	counterVolumeEnvelope++;
	counterSweep

	clockLength();
	clockVolume();
	clockSweep();
}