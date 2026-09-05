void delay_us(unsigned int a);
void delay_ms(unsigned int a);
void set_F_MHz(int f);

typedef struct { //структура, для работы с портами
 unsigned char ODR;
 unsigned char IDR;
 unsigned char DDR;
 unsigned char CR1;
 unsigned char CR2;
}PORT;

//-------------------------указатели для gpio портов--------------
#define GPIOA ((PORT*)0x5000)
#define GPIOB ((PORT*)0x5005)
#define GPIOC ((PORT*)0x500A)
#define GPIOD ((PORT*)0x500F)
#define GPIOE ((PORT*)0x5014)
#define GPIOF ((PORT*)0x5019)
#define GPIOG ((PORT*)0x501E)
