#include "stm32f1xx_hal.h" 
#define YOURS_UART huart2 //choose huart1 or huart2 
                          //huart1 PA9 huart2 PA2 for STM32F103C6
int onewire_init(void); 
void onewire_write_byte(uint8_t byte); 
uint8_t onewire_read_byte(void); 
int16_t read_temp(void); 
