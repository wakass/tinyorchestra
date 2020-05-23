#include <Arduino.h>
#include <Wire.h>
#include "../../tiny_player/include/tiny_player.h"
#include <tiny_conductor.h>
#include <tinyStatus.h>
#include <util/delay_basic.h>


#define GBHW_CYCLE_MS 1.0/4194304.0 *1e3
#define SONG_ song_hex
#define SONG_LEN song_hex_len
#include "../../song.h"
#include "../../megaman.h"
uint32_t prgCounter = 0;

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

void setup() {
    TCCR2A = 1 << WGM01; //CTC Mode
    TCCR2B = 6 << CS20; //256 prescale, ==> 62.5 Khz effective clock
    OCR2A = 122; //~512Hz
    TIMSK2 |= 1 << OCIE2A; //Enable interrupt fire on compare match
    
    Serial.begin(9600);
    Wire.begin();
    Serial.print(F("\n\nSerial is Open\n\n"));
    init_translation_table();

}


ISR (TIMER2_COMPA_vect) {
    sei(); //re-enable interrupts in order to send i2c data.
    metronomeTick();
}


typedef struct {
            long    elapsed;
            uint8_t addr;
            uint8_t val;
} gbs_instr;

#define TT translation_table
byte translation_table[255]; //only needs to count to 0xFF, or less, i guess
void init_translation_table(){
    //Map channel 1 to 2 for the moment
    TT[0x10] = NR20;
    TT[0x11] = NR21;
    TT[0x12] = NR22;
    TT[0x13] = NR23;
    TT[0x14] = NR24;

    TT[0x15] = NR20;
    TT[0x16] = NR21;
    TT[0x17] = NR22;
    TT[0x18] = NR23;
    TT[0x19] = NR24;
    TT[0x20] = 0;
}
void issue_instruction(uint8_t addr, uint8_t val) {
    Wire.beginTransmission(SLAVE_ADDR);
    byte reg_select = TT[addr] << 4; //Get instruction translation from GB land to orchestra land

    Wire.write(reg_select);
    Wire.write(val);

    Wire.endTransmission();
}
unsigned long loop_total_elapsed = 0;
void loop() {
     
    // incrementFrequency();
    // delay(10); //small delay or i2c is too fast for the human ear.
    
    unsigned long loop_single_elapsed = millis() - timeNow;

    //execute at ~60Hz, close to regular vblank in gb
    #define VBLANK_MS (1.0/59.0 * 1000.0)
    if (loop_single_elapsed >= VBLANK_MS){ //Effectively every 17ms
        timeNow = millis();
        
        gbs_instr instr;
        memcpy_P(&instr, &SONG_[prgCounter],sizeof(gbs_instr));
        uint32_t instruction_cycles_elapsed = instr.elapsed;
        
        //Do all instructions of which the total elapsed time is below our single loop time
        if (instruction_cycles_elapsed * GBHW_CYCLE_MS > loop_total_elapsed) {
            loop_total_elapsed += loop_single_elapsed;
        }
        else {
            loop_total_elapsed = loop_single_elapsed;
        }
        while (instruction_cycles_elapsed * GBHW_CYCLE_MS <= loop_total_elapsed){
            issue_instruction(instr.addr,instr.val);
            
            //fetch the next instruction
            
            prgCounter += sizeof(gbs_instr); //Ahead instuction bytes
            if (prgCounter > SONG_LEN) { prgCounter = 0;}
            memcpy_P(&instr, &SONG_[prgCounter],sizeof(gbs_instr));
            instruction_cycles_elapsed += instr.elapsed;
            _delay_loop_2(instr.elapsed<<2); 
            //each _delay_loop_2 takes 4 CPU cycles
            //Assuming gb runs at 4mhz and our conductor at 16mhz,
            //a 1 gb-cycle can be expressed as 4 of our cycles

                        
        }

    }
}


