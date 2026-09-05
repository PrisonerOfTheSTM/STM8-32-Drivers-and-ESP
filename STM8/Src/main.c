#include <iostm8s207.h>
#include <timerOneDelay.h>
#include <ds18b20.h>


void blink(void); //мигание светодиодом
void ds18b20_Acalibration(void);//функция считывания адрессов

float t1, t2, t3;// переменные для хранения температуры
char sensorAdd[3][8];//массив для хранения адрессов

main()
{
	
	PA_DDR |= 1<<6;
	PA_CR1 |= 1<<6;
	PA_CR2 |= 1<<6;
	
	sensor_init(GPIOA, 2);
	
	ds18b20_Acalibration();
	
	while (1) {
		temp_transmit();//производим процедуру преобразования температуры
		t1 = sensor_get_temp(sensorAdd[0]);
		t2 = sensor_get_temp(sensorAdd[1]);
		t3 = sensor_get_temp(sensorAdd[2]);
	}
}

void blink(void) {
	PA_ODR |= 1<<6;
	delay_ms(100);
	PA_ODR &= ~(1<<6);
	delay_ms(100);
	PA_ODR |= 1<<6;
	delay_ms(100);
	PA_ODR &= ~(1<<6);
	delay_ms(100);
}

void ds18b20_Acalibration(void) {
	int i, x;
	sensor_get_add();
	for(x = 0; x < 3; x++){
	sensor_get_add();
	for(i = 0; i < 8; i++) {
		sensorAdd[x][i] = read_curent__add();//побайтово считываем адресс
	}
	blink();
	delay_ms(10000);
}
}
