#include "commands.h" 
 
extern UART_HandleTypeDef YOURS_UART; 
float temp; 

int16_t read_temp(void)
{ 
  onewire_init(); 
  onewire_write_byte(0xCC); 
  onewire_write_byte(0x44); 
  onewire_init(); 
  onewire_write_byte(0xCC); 
  onewire_write_byte(0xBE); 
  uint8_t lsb = onewire_read_byte(); 
  uint8_t msb = onewire_read_byte(); 
  int twotemp = msb << 8 | lsb; 
  temp = twotemp / 16.0f; 
  return temp; 
} 
  
int onewire_init(void)
{ 
  YOURS_UART.Instance->BRR = 0x1D4C; 
  uint8_t start_byte = 0xF0; 
  uint8_t response = 0; 
  HAL_UART_Transmit(&YOURS_UART, &start_byte, 1, 10); 
  HAL_UART_Receive(&YOURS_UART, &response, 1, 10); 
      return response; 
  } 
 
void onewire_write_byte(uint8_t byte) 
{ 
  YOURS_UART.Instance->BRR = 0x271; 
  for (uint8_t i = 0; i < 8; i++) 
  { 
    uint8_t bit = (byte >> i) & 0x01; 
    uint8_t tx_byte = bit ? 0xFF : 0x00; 
    HAL_UART_Transmit(&YOURS_UART, &tx_byte, 1, 10); 
  } 
} 
 
uint8_t onewire_read_byte(void) 
{ 
  YOURS_UART.Instance->BRR = 0x271; 
  uint8_t result = 0; 
  uint8_t tx_byte = 0xFF; 
  uint8_t rx_byte = 0; 
  for (uint8_t i = 0; i < 8; i++) 
  { 
    HAL_UART_Transmit(&YOURS_UART, &tx_byte, 1, 10); 
    HAL_UART_Receive(&YOURS_UART, &rx_byte, 1, 10); 
    if (rx_byte == 0xFF)
    { 
      result |= (1 << i); 
    } 
  } 
  return result; 
} 
 
uint8_t onewire_read_byte_rom(void) 
{ 
  YOURS_UART.Instance->BRR = 0x271; 
  uint8_t result = 0; 
  uint8_t tx_byte = 0xFF; 
  uint8_t rx_byte = 0; 
  for (uint8_t i = 0; i < 8; i++) 
  { 
    
    HAL_UART_Transmit(&YOURS_UART, &tx_byte, 1, 10); 
    HAL_UART_Receive(&YOURS_UART, &rx_byte, 1, 10); 
    if (rx_byte == 0xFF)
    { 
      result |= (1 << i); 
    } 
  } 
  return result; 
} 
