#include <iostm8s207.h>
#include <timerOneDelay.h>
#include <ds18b20.h>

int Data[9]; 
char _add[8];
unsigned char PIN;
char num_byte_addres;

void del3us(int tt) {
 int m;
 for(m = 0; m < tt; m++);
}

char read_curent__add(void) {
	char arry;
	arry = _add[num_byte_addres];
	if (num_byte_addres < 7) num_byte_addres++;
	else num_byte_addres = 0;
	return arry;
}

char* sensor_get_add(void) {
	int w;
	static char arry[8];
	sensor_rst();
  sensor_WriteByte(0x33);
	for(w = 0; w < 8; w++){
		_add[w] = sensor_ReadByte();
		arry[w] = _add[w];
}
	return arry;
	
	//return arry;
}

void sensor_init(PORT* p, char pin) {
	
 CLK_CKDIVR &= ~(1<<4 | 1<<3); //разгоняем stm!!!
 set_F_MHz(16); //частота переферии 16 мгц
 p -> DDR |= (1<<pin);
 p -> CR1 |= (0x00);
 p -> CR2 |= (1<<pin);
 PIN = pin;
}

void temp_transmit(void) {
 sensor_rst();
 sensor_WriteByte(0xCC);
 sensor_WriteByte(0x44); 
 delay_ms(1000);
}
 
float sensor_get_temp(char*a) {
 int m;
 float t = 0.0;
 int i;
 
 sensor_rst(); 
 //sensor_WriteByte(0xCC); 
 sensor_WriteByte(0x55);
 for(i = 0; i < 8; i++) sensor_WriteByte(*(a+i));
 sensor_WriteByte(0xBE);
 for(m = 0; m < 8; m++) Data[m] = sensor_ReadByte(); 
 t = ((Data[1] & 0x07) << 8 | Data[0]) * 0.0625;
 if ((Data[1] & 1<<7) == 1<<7) t = t * -1.0;
 return t;
}
 
int sensor_rst(void) { 
 
 int st, s; 
 PA_ODR &= ~(1<<PIN);//подаём низкий уровень 
 delay_us(500); 
 PA_ODR |= (1<<PIN);//подаём высокий уровень 
 delay_us(65); 
 s = PC_IDR & 1<<PIN; 
 if (s == 4) { 
 st = 0; 
} 
 else { 
 st = 1; 
} 
 //st = PC_IDR & 1<<2; 
 delay_us(480); 
 return st; 
} 

int sensor_ReadBit(void) { 
 char bit = 0; 
 char s = 0; 
 PA_ODR &= ~(1<<PIN);//подаём низкий уровень 
 del3us(1); //delay_us(1);
 PA_ODR |= (1<<PIN); //высокий уровень 
 del3us(1);
 s = PA_IDR & 1<<PIN; 
 delay_us(35);
 return s>>PIN; 
} 
 
int sensor_ReadByte(void) { 
 char data = 0; 
 int i; 
 for (i = 0; i <= 7; i++) {
   data |= sensor_ReadBit() << i; 
 }
 return data; 
} 
 
int sensor_WriteBit(int bit) { 
 PA_ODR &= ~(1<<PIN);//подаём низкий уровень 
 delay_us(bit ? 1 : 63); 
 PA_ODR |= (1<<PIN); //высокий уровень
 delay_us(bit ? 63 : 1); 
 del3us(2);
} 
int sensor_WriteByte(int byte) { 
 int i; 
 for(i = 0; i < 8; i++) { 
  sensor_WriteBit(byte >> i & 1); 
  delay_us(5); 
 } 
}
