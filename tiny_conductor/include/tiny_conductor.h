#define SLAVE_ADDR 100

void init_translation_table();
void metronomeTick();
void incrementFrequency();
void issue_instruction(uint8_t addr, uint8_t val);