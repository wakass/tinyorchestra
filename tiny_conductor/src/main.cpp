#include <Arduino.h>
#include <Wire.h>
#include "../../tiny_player/include/tiny_player.h"
#include <tiny_conductor.h>
#include <tinyStatus.h>

#define GBHW_CYCLE_MS 1.0/4194304.0 *1e3
#include "../../song.h"
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
    init_translation_table();
    initSound();
    printTinyStatus();

    setupSound();
    printTinyStatus();

}


// ISR (TIMER2_COMPA_vect) {
//     sei(); //re-enable interrupts in order to send i2c data.
//     metronomeTick();
// }


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
    // if (addr == 0x23) Serial.println(addr);
    // Serial.println(val);
    
    Wire.beginTransmission(SLAVE_ADDR);
    byte reg_select = TT[addr] << 4; //Get instruction translation from GB land to orchestra land

    Wire.write(reg_select);
    Wire.write(val);

    Wire.endTransmission();
}

void loop() {
     
    // incrementFrequency();
    // delay(10); //small delay or i2c is too fast for the human ear.
    
    unsigned long elapsed = millis() - timeNow;

    if (elapsed >= (1.0/60.0 * 1000.0 )){
        timeNow = millis();
        gbs_instr instr;
        memcpy_P(&instr, &song_hex[prgCounter],sizeof(gbs_instr));
        uint32_t instruction_cycles_elapsed = instr.elapsed;

        while (instruction_cycles_elapsed * GBHW_CYCLE_MS < elapsed){
            issue_instruction(instr.addr,instr.val);
            
            //fetch the next instruction
            prgCounter += sizeof(gbs_instr); //Ahead instuction bytes
            memcpy_P(&instr, &song_hex[prgCounter],sizeof(gbs_instr));
            instruction_cycles_elapsed += instr.elapsed;
            // Serial.print("Cyc elapsed: ");
            // Serial.println(instruction_cycles_elapsed);
            // Serial.print("elapsed: ");
            // Serial.println(elapsed);
            
        }
        // Serial.print("prgC: ");
        // Serial.println(prgCounter);
        // Serial.print("elpsd: ");
        // Serial.println(elapsed);
        // Serial.print("ins cycl: ");
        // Serial.println(instruction_cycles_elapsed);

    }
}


